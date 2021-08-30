#include <fstream>
#include <stdexcept>

#include "nemesis/source/source.hpp"
#include "nemesis/tokenizer/token.hpp"

namespace nemesis {
    source_location::source_location() : line{0}, column{0}, filename() {}

    source_location::source_location(unsigned lineno, unsigned colno, utf8::span file) :
        line(lineno),
        column(colno),
        filename(file)
    {}
    
    bool source_location::valid() const { return filename.size() > 0 && line > 0 && column > 0; }

    source_location source_range::begin() const { return source_location(bline, bcolumn, filename); }

    source_location source_range::end() const { return source_location(eline, ecolumn, filename); }

    source_range& source_range::begin(source_location loc)
    {
        bline = loc.line;
        bcolumn = loc.column;
        return *this;
    }

    source_range& source_range::end(source_location loc)
    {
        eline = loc.line;
        ecolumn = loc.column;
        return *this;
    }

    source_range::source_range() :
        bline(0),
        eline(0),
        bcolumn(0),
        ecolumn(0),
        filename()
    {}

    source_range::source_range(unsigned bline, unsigned bcol, unsigned eline, unsigned ecol, utf8::span file) :
        bline(bline),
        eline(eline),
        bcolumn(bcol),
        ecolumn(ecol),
        filename(file)
    {}


    source_range::source_range(source_location begin, unsigned cols) :
        bline(begin.line),
        eline(begin.line),
        bcolumn(begin.column),
        ecolumn(begin.column + cols),
        filename(begin.filename)
    {}

    source_range::source_range(source_location begin, source_location end) :
        bline(begin.line),
        eline(end.line),
        bcolumn(begin.column),
        ecolumn(end.column),
        filename(begin.filename)
    {}

    source_file::buffer::buffer(int size) :
        data{new byte[size]},
        size{size}
    {}

    source_file::buffer::~buffer()
    {
        size = 0;
        if (data) delete[] data;
    }

    source_file::source_file(source_handler& handler, utf8::span name, int size) :
        handler_{&handler},
        name_{name},
        buffer_(size),
        line_table_{},
        ast_(nullptr)
    {}

    source_handler& source_file::get_source_handler() const
    {
        return *handler_;
    }

    utf8::span source_file::name() const
    {
        return name_;
    }

    utf8::span source_file::source() const
    {
        return utf8::span(buffer_.data, buffer_.size);
    }

    utf8::span source_file::line(unsigned index) const
    {
        --index;

        if (index >= line_table_.size()) {
            throw std::out_of_range("source_file::line(): line index is out of range");
        }

        return line_table_.at(index);
    }

    std::size_t source_file::lines_count() const { return line_table_.size(); }

    utf8::span source_file::range(source_range rng) const
    {
        if (rng.bline - 1 >= line_table_.size()) {
            throw std::out_of_range("source_file::line(): line index is out of range");
        }

        if (rng.eline < 1 || rng.eline - 1 >= line_table_.size()) rng.eline = line_table_.size();

        utf8::span bline = line_table_.at(rng.bline - 1), eline = line_table_.at(rng.eline - 1);
        utf8::span::iterator begin = bline.begin(), end = eline.begin();

        if (rng.bcolumn < 1) rng.bcolumn = 1;
        if (rng.ecolumn < 1 || rng.ecolumn > static_cast<unsigned>(eline.width())) rng.ecolumn = eline.width();

        for (unsigned bcol = 1; bcol < rng.bcolumn; bcol += utf8::width(*begin), ++begin);
        for (unsigned ecol = 1; ecol < rng.ecolumn; ecol += utf8::width(*end), ++end);

        return utf8::span(begin, end);
    }

    std::shared_ptr<ast::node> source_file::ast() const { return ast_; }

    void source_file::ast(std::shared_ptr<ast::node> ast) { ast_ = ast; }

    bool source_file::has_type(filetype type) const
    {
        auto name = name_.string();
        auto extension = name.substr(name.find_last_of('.') + 1);
        
        switch (type) {
            case filetype::header:
                return extension == "h" || extension == "hpp";
            case filetype::cpp:
                return extension == "cpp" || extension == "cxx" || extension == "cc";
            case filetype::nemesis:
                return extension == "ns";
            default:
                break;
        }

        return true;
    }

    source_handler& source_handler::instance()
    {
        static source_handler handler;
        return handler;
    }

    source_handler::source_handler() :
        files_{}
    {}

    source_handler::~source_handler()
    {
        for (auto pair : files_) {
            if (pair.second) delete pair.second;
        }
    }
        
    bool source_handler::load(utf8::span filename)
    {
        std::ifstream stream(reinterpret_cast<const char*>(filename.data()), std::ios_base::binary);

        if (!stream.is_open()) return false;

        auto fbuf = stream.rdbuf();
        int size = fbuf->pubseekoff(0, stream.end, stream.in);
        fbuf->pubseekpos(0, std::ios_base::in);

        source_file *source = new source_file(*this, filename, size);
        fbuf->sgetn(reinterpret_cast<char*>(source->buffer_.data), size);

        stream.close();

        if (source->has_type(source_file::filetype::cpp) || source->has_type(source_file::filetype::header)) cpp_files_.emplace(filename, source);
        else if (source->has_type(source_file::filetype::nemesis)) files_.emplace(filename, source);

        return true;
    }
        
    void source_handler::remove(utf8::span filename)
    {
        auto res = files_.find(filename);
        if (res == files_.end()) {
            res = cpp_files_.find(filename);
            if (res == cpp_files_.end()) throw std::invalid_argument("source_handler::get(): file is not owned by source handler");
        }

        delete res->second;
        files_.erase(filename);
    }
        
    source_file& source_handler::get(utf8::span filename) const
    {
        auto res = files_.find(filename);
        if (res == files_.end()) {
            res = cpp_files_.find(filename);
            if (res == cpp_files_.end()) throw std::invalid_argument("source_handler::get(): file is not owned by source handler");
        }

        return *(res->second);
    }

    const std::unordered_map<utf8::span, source_file*>& source_handler::sources() const { return files_; }

    const std::unordered_map<utf8::span, source_file*>& source_handler::cppsources() const { return cpp_files_; }
}

std::ostream& operator<<(std::ostream& os, nemesis::source_location loc)
{
    if (!loc.valid()) return os << "null:0:0";
    return os << loc.filename << ":" << loc.line << ":" << loc.column;
}