#include <numeric>
#include <stdexcept>
#include <sstream>

#include "nemesis/diagnostics/diagnostic.hpp"

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux__)
#include <sys/ioctl.h>
#endif 

#define __NEMESIS_DIAGNOSTIC_EXPLANATION__ 0

namespace nemesis {
    diagnostic::builder& diagnostic::builder::severity(enum severity severity)
    {
        diag_.severity_ = severity;
        return *this;
    }

    diagnostic::builder& diagnostic::builder::small(bool flag)
    {
        diag_.small_ = flag;
        return *this;
    }

    diagnostic::builder& diagnostic::builder::location(source_location location)
    {
        diag_.location_ = location;
        return *this;
    }

    diagnostic::builder& diagnostic::builder::message(std::string message)
    {
        diag_.message_ = message;
        return *this;
    }

    diagnostic::builder& diagnostic::builder::explanation(std::string message)
    {
        diag_.explanation_ = message;
        return *this;
    }

    diagnostic::builder& diagnostic::builder::highlight(source_range range, std::string hint, enum diagnostic::highlighter::mode mode)
    {
        diag_.highlighted_.push_back(diagnostic::highlighter { range, hint, mode });
        return *this;
    }

    diagnostic::builder& diagnostic::builder::highlight(source_range range, enum diagnostic::highlighter::mode mode)
    {
        diag_.highlighted_.push_back(diagnostic::highlighter { range, {}, mode });
        return *this;
    }

    diagnostic::builder& diagnostic::builder::note(source_range range, std::string message)
    {
        diag_.notes_.push_back(diagnostic::highlighter { range, message, diagnostic::highlighter::mode::heavy });
        return *this;
    }

    diagnostic::builder& diagnostic::builder::replacement(source_range range, std::string fix, std::string message)
    {
        diag_.fixes_.push_back(diagnostic::fixman { range, fix, message, diagnostic::fixman::action::replace });
        return *this;
    }

    diagnostic::builder& diagnostic::builder::insertion(source_range range, std::string fix, std::string message)
    {
        diag_.fixes_.push_back(diagnostic::fixman { range, fix, message, diagnostic::fixman::action::insert });
        return *this;
    }

    diagnostic::builder& diagnostic::builder::removal(source_range range, std::string message)
    {
        diag_.fixes_.push_back(diagnostic::fixman { range, "", message, diagnostic::fixman::action::remove });
        return *this;
    }

    diagnostic diagnostic::builder::build() const { return diag_; }

    enum diagnostic::severity diagnostic::severity() const { return severity_; }

    bool diagnostic::small() const { return small_; }

    source_location diagnostic::location() const { return location_; }
        
    std::string diagnostic::message() const { return message_; }

    std::string diagnostic::explanation() const { return explanation_; }
        
    std::vector<diagnostic::highlighter> diagnostic::highlighted() const { return highlighted_; }

    std::vector<diagnostic::highlighter> diagnostic::notes() const { return notes_; }

    std::vector<diagnostic::fixman> diagnostic::fixes() const { return fixes_; }

    diagnostic_publisher::~diagnostic_publisher() {}

    unsigned diagnostic_publisher::errors() const { return errors_; }

    unsigned diagnostic_publisher::warnings() const { return warnings_; }

    void diagnostic_publisher::attach(diagnostic_subscriber& subscriber)
    { 
        subscribers_.insert(&subscriber);
    }
    
    void diagnostic_publisher::detach(diagnostic_subscriber& subscriber) { subscribers_.erase(&subscriber); }
    
    void diagnostic_publisher::publish(diagnostic diag)
    {
        if (diag.severity() == diagnostic::severity::error) {
            ++errors_;
        }
        else if (diag.severity() == diagnostic::severity::warning) {
            ++warnings_;
        }

        for (diagnostic_subscriber *subscriber : subscribers_) {
            subscriber->handle(diag);
        }
    }

    diagnostic_printer::diagnostic_printer(std::ostream& stream) :
        diagnostic_subscriber{},
        stream_{stream}
    {}

    namespace impl {
        void get_terminal_size(unsigned& width, unsigned& height) 
        {
#if defined(_WIN32)
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
            width = static_cast<unsigned>(csbi.dwSize.X);
            height = static_cast<unsigned>(csbi.dwSize.Y);
#elif defined(__unix__) || defined(__unix) || defined(unix)
            struct winsize w;
            ioctl(fileno(stdout), TIOCGWINSZ, &w);
            width = static_cast<unsigned>(w.ws_col);
            height = static_cast<unsigned>(w.ws_row);
#endif
        }

        std::pair<int, int> line_ranges_of(const diagnostic& diag, std::vector<diagnostic::highlighter>& highlighted)
        {
            unsigned minline = std::numeric_limits<unsigned>::max(), maxline = 0;

            for (const diagnostic::highlighter& h : diag.highlighted()) {
                if (h.range.bline < minline) minline = h.range.bline;
                if (h.range.eline > maxline) maxline = h.range.eline;

                for (unsigned line = h.range.bline; line <= h.range.eline; ++line) {
                    diagnostic::highlighter copy = h;
                    // from beginning of the line
                    if (copy.range.bline != line) {
                        copy.range.bline = line;
                        copy.range.bcolumn = 1;
                    }
                    // to the end of the line
                    if (copy.range.eline != line) {
                        copy.range.eline = line;
                        copy.range.ecolumn = 0;
                    }
                    // inlined message is appended at the end of the highlighted range
                    if (copy.range.eline != h.range.eline && copy.range.ecolumn != h.range.ecolumn) {
                        copy.hint.clear();
                    }
                    // adds to the result
                    highlighted.push_back(copy);
                }
            }

            return std::pair<int, int>(minline, maxline);
        }


        void mark(const utf8::span& line, std::string& light, std::string& heavy, diagnostic::highlighter h, char marker, bool inlined = true)
        {
            if (h.range.ecolumn < 1 || h.range.ecolumn > static_cast<unsigned>(line.width())) {
                h.range.ecolumn = line.width() + 1;
            }

            if (h.range.bcolumn == h.range.ecolumn) {
                ++h.range.ecolumn;
            }

            --h.range.bcolumn;
            --h.range.ecolumn;

            unsigned col = 0;
            utf8::span::iterator it = line.begin();

            while (col < h.range.bcolumn) {
                col += utf8::width(*it);
                ++it;
            }

            while (*it == ' ') {
                ++h.range.bcolumn;
                ++it;
            }

            for (std::size_t i = h.range.bcolumn; i < h.range.ecolumn; ++i) {
                if (h.mode == diagnostic::highlighter::mode::heavy) {
                    heavy.at(i) = marker;
                }
                else {
                    heavy.at(i) = '-';
                    light.at(i) = marker;
                }
            }

            if (!inlined) return;

            // print inline hint if there is enough space
            if (h.range.ecolumn + 1 + h.hint.length() < heavy.size()) {
                for (std::size_t i = 0;  i < h.hint.length(); ++i) {
                    heavy.at(h.range.ecolumn + 1 + i) = h.hint.at(i);
                }
            }
        }


        // exclusive line range [bcol, ecol)
        std::string highlight(utf8::span line, const std::string& light, const std::string& heavy, const char* color)
        {
            std::ostringstream oss;
            int col = 0;

            for (codepoint c : line) {
                byte encoded[4] = { 0 };
                utf8::encode(c, encoded);

                if (heavy[col] == '^') {
                    oss << color << encoded << color::reset;
                }
                else if (light[col] == '^') {
                    oss << impl::color::white << encoded << color::reset;
                }
                else {
                    oss << encoded;
                }

                col += utf8::width(c);
            }

            return oss.str();
        }

        std::string pointers(std::string underline, const char* color)
        {
            std::string result;
            auto i = underline.begin();

            while (i != underline.end()) {
                switch (*i) {
                    case '^':
                        result += color;
                        for (; i != underline.end() && *i == '^'; ++i) result += '^';
                        result += impl::color::reset;
                        break;
                    case '-':
                        result += impl::color::white;
                        for (; i != underline.end() && *i == '-'; ++i) result += '-';
                        result += impl::color::reset;
                        break;
                    case ' ':
                        result += *i;
                        ++i;
                    default:
                        result += color;
                        for (; i != underline.end() && *i != '^' && *i != '-' && *i != ' '; ++i) result += *i;
                        result += impl::color::reset;
                        break;
                }
            }

            return result;
        }

        // print a fix replacement inside source text
        std::string fix(utf8::span line, diagnostic::fixman fixman)
        {
            if (fixman.range.eline != fixman.range.bline) {
                fixman.range.eline = fixman.range.bline;
            }

            if (fixman.action == diagnostic::fixman::action::insert) {
                    fixman.range.ecolumn = fixman.range.bcolumn;
            }

            std::ostringstream oss;
            
            if (fixman.range.ecolumn < 1) {
                oss << color::green << fixman.fix << color::reset << line;
            }
            else if (fixman.range.bcolumn > static_cast<unsigned>(line.width())) {
                oss << line << color::green << fixman.fix << color::reset;
            }
            else {
                utf8::span::iterator it = line.begin();
                unsigned col = 1;

                while (col < fixman.range.bcolumn) {
                    byte encoded[4] = { 0 };
                    utf8::encode(*it, encoded);
                    oss << encoded;
                    col += utf8::width(*it);
                    ++it;
                }

                if (fixman.action != diagnostic::fixman::action::remove) {
                    oss << color::green << fixman.fix << color::reset;
                }

                if (fixman.action == diagnostic::fixman::action::remove ||
                    fixman.action == diagnostic::fixman::action::replace) {
                    while (col < fixman.range.ecolumn) {
                        col += utf8::width(*it);
                        ++it;
                    }
                }

                while (it < line.end()) {
                    byte encoded[4] = { 0 };
                    utf8::encode(*it, encoded);
                    oss << encoded;
                    col += utf8::width(*it);
                    ++it;
                }
            }

            return oss.str();
        }

        std::string message(const std::string& input)
        {
            std::string output;

            for (auto it = input.begin(); it != input.end();) {
                if (*it == '`') {
                    it++;
                    output += impl::color::white;
                    while (it != input.end() && *it != '`') output += *(it++);
                    if (it < input.end() - 1 && *it == '`' && *(it + 1) == '`') output += *(it++);
                    output += impl::color::reset;
                    if (it != input.end()) ++it;
                }
                else {
                    output += *(it++);
                }
            }

            return output;
        }

        void justify(const std::string& message, std::ostringstream& oss, unsigned width)
        {
            unsigned line_width = 0;

            oss << "  ";

            for (const char* ptr = message.data(); ptr < message.data() + message.size();) {
                while (ptr < message.data() + message.size() && (*ptr == ' ' || *ptr == '\t' || *ptr == '\v' || *ptr == '\n')) ++ptr;
                
                if (strncmp(impl::color::reset, ptr, sizeof(impl::color::reset) - 1) == 0) {
                    oss << impl::color::reset;
                    ptr += sizeof(impl::color::reset) - 1;
                }
                else if (strncmp(impl::color::white, ptr, sizeof(impl::color::white) - 1) == 0) {
                    oss << impl::color::white;
                    ptr += sizeof(impl::color::white) - 1;
                }

                const char *wordend = ptr;

                while (wordend < message.data() + message.size() && *wordend != ' ' && *wordend != '\t' && *wordend != '\v' && *wordend != '\n') ++wordend;

                if (strncmp("•", ptr, wordend - ptr) == 0) {
                    oss << '\n' << "   • ";
                    line_width = 3;
                }
                else if (wordend - ptr > 1 && *ptr == '\\' && *(ptr + 1) >= '0' && *(ptr + 1) <= '9') {
                    oss << "\n  " << std::string(static_cast<std::size_t>(*(ptr + 1) - '0'), ' ');
                    line_width = *(ptr + 1) - '0';
                }
                else if (*ptr == '\\') {
                    oss << "\n  ";
                    line_width = 0;
                }
                else if (line_width + wordend - ptr > width) {
                    oss << '\n' << "  ";
                    for (const char* p = ptr; p < wordend; ++p) oss << *p;
                    oss << ' ';
                    line_width = wordend - ptr + 1;
                }
                else {
                    for (const char* p = ptr; p < wordend; ++p) oss << *p;
                    oss << ' ';
                    line_width += wordend - ptr + 1;
                }

                ptr = wordend;
            }

            oss << "\n\n";
        }

        std::string nice(const diagnostic& diag)
        {
            constexpr unsigned max_width = /*96*/112;
            std::ostringstream oss;

            if (!diag.location().valid()) 
            {
                switch (diag.severity()) {
                    case diagnostic::severity::error:
                        oss << "• " << color::red << "error" << color::reset << ": ";
                        break;
                    case diagnostic::severity::warning:
                        oss << "• " << color::yellow << "warning" << color::reset << ": ";
                        break;
                    case diagnostic::severity::none:
                        oss << "• ";
                        break;
                }

                oss << message(diag.message()) << "\n";
            }
            else {
                source_file& file = source_handler::instance().get(diag.location().filename);
                std::string title;
                const char* color;
                    
                switch (diag.severity()) {
                    case diagnostic::severity::error:
                        color = color::red;
                        oss << "• " << color::red << "error" << color::reset << " in file " << impl::color::white << file.name() << impl::color::reset << " at line " << impl::color::white << diag.location().line << impl::color::reset << '\n';
                        break;
                    case diagnostic::severity::warning:
                        color = color::yellow;
                        oss << "• " << color::yellow << "warning" << color::reset << " in file " << impl::color::white << file.name() << impl::color::reset << " at line " << impl::color::white << diag.location().line << impl::color::reset << '\n';
                        break;
                    case diagnostic::severity::none:
                        throw std::invalid_argument("impl::nice: diagnostic without severity associated to source location");
                        break;
                }

                oss << '\n';

                justify(message(diag.message()), oss, max_width);

                if (!diag.highlighted().empty()) {
                    std::vector<diagnostic::highlighter> highlighted;
                    unsigned minline, maxline;
                    std::tie(minline, maxline) = impl::line_ranges_of(diag, highlighted);
                    
                    if (minline > 1) --minline;
                    if (maxline < file.lines_count()) ++maxline;

                    unsigned max_line_width = max_width;

                    for (unsigned line = minline; line <= maxline; ++line) {
                        unsigned int w = file.line(line).width();
                        if (w > max_line_width) max_line_width = w;
                    }

                    for (unsigned line = minline; line <= maxline; ++line) {
                        if (diag.small() && maxline > minline + 1 && line > minline + 1 && line < maxline - 1) {
                            oss << "   .\n   .\n   .\n\n";
                            line = maxline - 2;
                            continue;
                        }

                        utf8::span linestr = file.line(line);
                        std::string light(max_line_width + 1, ' '), heavy(light);
                        bool heavymarked = false, lightmarked = false;

                        for (const diagnostic::highlighter& h : highlighted) {
                            if (h.range.bline == line) {
                                if (h.mode == diagnostic::highlighter::mode::heavy) heavymarked = true;
                                else lightmarked = true;
                                impl::mark(linestr, light, heavy, h, '^');
                            }
                        }

                        std::ostringstream tmp;
                        
                        tmp << "  " << line << std::string(std::to_string(file.lines_count()).size() - std::to_string(line).size() + 1, ' ');

                        if (heavymarked) {
                            std::string prefix1 = tmp.str();
                            std::string prefix2(prefix1.size(), ' ');

                            oss << color << prefix1 << "> " << impl::color::reset << highlight(linestr, light, heavy, color) << "\n";
                            oss << prefix2 << "| " << pointers(heavy, color) << "\n";
                        }
                        else if (lightmarked) {
                            std::string prefix1 = tmp.str();
                            std::string prefix2(prefix1.size(), ' ');

                            oss << impl::color::white << prefix1 << "> " << impl::color::reset << highlight(linestr, light, heavy, color) << "\n";
                            oss << prefix2 << "| " << pointers(heavy, color) << "\n";
                        }
                        else
                        {
                            std::string prefix1 = tmp.str();

                            oss << prefix1 << "| " << highlight(linestr, light, heavy, color) << "\n";
                        }
                    }

                    oss << '\n';
                }

                for (diagnostic::highlighter note : diag.notes()) {
                    source_file& other = source_handler::instance().get(note.range.filename);
                    utf8::span linestr = other.line(note.range.bline);
                    unsigned max_line_width = linestr.width() + 1;
                    std::ostringstream tmp;

                    if (max_line_width < max_width) max_line_width = max_width;

                    std::string heavy(max_line_width + 1, ' '), light(max_line_width + 1, ' ');

                    impl::mark(linestr, light, heavy, note, '^', false);

                    tmp << "  " << note.range.bline << std::string(std::to_string(other.lines_count()).size() - std::to_string(note.range.bline).size() + 1, ' ');
                    
                    std::string prefix1 = tmp.str();
                    std::string prefix2(prefix1.size(), ' ');

                    oss << "• " << color::blue << "note" << color::reset << " in file " << impl::color::white << note.range.filename << impl::color::reset << " at line " << impl::color::white << note.range.bline << impl::color::reset << "\n\n";
                    
                    justify(message(note.hint), oss, max_width);

                    if (note.range.bline - 1 > 0) {
                        oss << "  " << note.range.bline - 1 << std::string(std::to_string(other.lines_count()).size() - std::to_string(note.range.bline - 1).size() + 1, ' ') << "| " << other.line(note.range.bline - 1) << "\n";
                    }

                    oss << impl::color::blue << prefix1 << "> " << impl::color::reset << impl::highlight(linestr, light, heavy, impl::color::blue) << "\n";
                    oss << prefix2 << "| " << pointers(heavy, impl::color::blue) << "\n";

                    if (note.range.bline + 1 <= other.lines_count()) {
                        oss << "  " << note.range.bline + 1 << std::string(std::to_string(other.lines_count()).size() - std::to_string(note.range.bline + 1).size() + 1, ' ') << "| " << other.line(note.range.bline + 1) << "\n";
                    }
                }

                if (!diag.notes().empty()) oss << "\n";

                for (diagnostic::fixman fixman : diag.fixes()) {
                    oss << "• " << color::green << "hint" << color::reset << " in file " << impl::color::white << fixman.range.filename << impl::color::reset << " at line " << impl::color::white << fixman.range.bline << impl::color::reset << "\n\n";

                    std::ostringstream tmp;

                    tmp << "  " << fixman.range.bline << std::string(std::to_string(file.lines_count()).size() - std::to_string(fixman.range.bline).size() + 1, ' ');
                    
                    std::string prefix1 = tmp.str();
                    std::string prefix2(prefix1.size(), ' ');
                    
                    justify(message(fixman.hint), oss, max_width);
                    oss << prefix1 << "| " << fix(file.line(fixman.range.bline), fixman) << "\n";
                }

                if (!diag.fixes().empty()) oss << "\n";
            }

#if defined(__NEMESIS_DIAGNOSTIC_EXPLANATION__) && __NEMESIS_DIAGNOSTIC_EXPLANATION__
            if (!diag.explanation().empty()) justify(message(diag.explanation()), oss, max_width);
#endif

            return oss.str();
        }

        std::string nice_gccstyle(const diagnostic& diag)
        {
            constexpr unsigned max_width = 96;
            std::ostringstream oss;

            if (!diag.location().valid()) 
            {
                switch (diag.severity()) {
                    case diagnostic::severity::error:
                        oss << color::red << "error" << color::reset << ": ";
                        break;
                    case diagnostic::severity::warning:
                        oss << color::yellow << "warning" << color::reset << ": ";
                        break;
                    case diagnostic::severity::none:
                        break;
                }

                oss << message(diag.message()) << "\n";
            }
            else {
                source_file& file = source_handler::instance().get(diag.location().filename);
                std::string title;
                const char* color;
                    
                switch (diag.severity()) {
                    case diagnostic::severity::error:
                        color = color::red;
                        oss << impl::color::white << file.name() << impl::color::reset << ":" << impl::color::white << diag.location().line << impl::color::reset << ":" << impl::color::white << diag.location().column << impl::color::reset << ": " << color::red << "error" << color::reset << ": " ;
                        break;
                    case diagnostic::severity::warning:
                        color = color::yellow;
                        oss << impl::color::white << file.name() << impl::color::reset << ":" << impl::color::white << diag.location().line << impl::color::reset << ":" << impl::color::white << diag.location().column << impl::color::reset << ": " << color::yellow << "warning" << color::reset << ": ";
                        break;
                    case diagnostic::severity::none:
                        throw std::invalid_argument("impl::nice: diagnostic without severity associated to source location");
                        break;
                }

                oss << message(diag.message()) << '\n';

                if (!diag.highlighted().empty()) {
                    std::vector<diagnostic::highlighter> highlighted;
                    unsigned minline, maxline;
                    std::tie(minline, maxline) = impl::line_ranges_of(diag, highlighted);
                    
                    if (minline > 1) --minline;
                    if (maxline < file.lines_count()) ++maxline;

                    for (unsigned line = minline; line <= maxline; ++line) {
                        if (diag.small() && maxline > minline + 1 && line > minline + 1 && line < maxline - 1) {
                            oss << "   .\n   .\n   .\n\n";
                            line = maxline - 2;
                            continue;
                        }

                        utf8::span linestr = file.line(line);
                        std::string light(max_width + 1, ' '), heavy(light);
                        bool heavymarked = false, lightmarked = false;

                        for (const diagnostic::highlighter& h : highlighted) {
                            if (h.range.bline == line) {
                                if (h.mode == diagnostic::highlighter::mode::heavy) heavymarked = true;
                                else lightmarked = true;
                                impl::mark(linestr, light, heavy, h, '^');
                            }
                        }

                        std::ostringstream tmp;
                        
                        tmp << "  " << line << std::string(std::to_string(file.lines_count()).size() - std::to_string(line).size() + 1, ' ');

                        if (heavymarked) {
                            std::string prefix1 = tmp.str();
                            std::string prefix2(prefix1.size(), ' ');

                            oss << color << prefix1 << "> " << impl::color::reset << highlight(linestr, light, heavy, color) << "\n";
                            oss << prefix2 << "| " << pointers(heavy, color) << "\n";
                        }
                        else if (lightmarked) {
                            std::string prefix1 = tmp.str();
                            std::string prefix2(prefix1.size(), ' ');

                            oss << impl::color::white << prefix1 << "> " << impl::color::reset << highlight(linestr, light, heavy, color) << "\n";
                            oss << prefix2 << "| " << pointers(heavy, color) << "\n";
                        }
                        else
                        {
                            std::string prefix1 = tmp.str();

                            oss << prefix1 << "| " << highlight(linestr, light, heavy, color) << "\n";
                        }
                    }
                }

                for (diagnostic::highlighter note : diag.notes()) {
                    source_file& other = source_handler::instance().get(note.range.filename);
                    utf8::span linestr = other.line(note.range.bline);
                    std::string light(max_width + 1, ' '), heavy(light);
                    std::ostringstream tmp;

                    impl::mark(linestr, light, heavy, note, '^', false);

                    tmp << "  " << note.range.bline << std::string(std::to_string(other.lines_count()).size() - std::to_string(note.range.bline).size() + 1, ' ');
                    
                    std::string prefix1 = tmp.str();
                    std::string prefix2(prefix1.size(), ' ');

                    oss << impl::color::white << note.range.filename << impl::color::reset << ":" << impl::color::white << note.range.bline << impl::color::reset << ":" << impl::color::white << note.range.bcolumn << impl::color::reset << ": " << color::blue << "note" << color::reset << ": " << message(note.hint) << '\n';

                    if (note.range.bline - 1 > 0) {
                        oss << "  " << note.range.bline - 1 << std::string(std::to_string(other.lines_count()).size() - std::to_string(note.range.bline - 1).size() + 1, ' ') << "| " << other.line(note.range.bline - 1) << "\n";
                    }

                    oss << impl::color::blue << prefix1 << "> " << impl::color::reset << impl::highlight(linestr, light, heavy, impl::color::blue) << "\n";
                    oss << prefix2 << "| " << pointers(heavy, impl::color::blue) << "\n";

                    if (note.range.bline + 1 <= other.lines_count()) {
                        oss << "  " << note.range.bline + 1 << std::string(std::to_string(other.lines_count()).size() - std::to_string(note.range.bline + 1).size() + 1, ' ') << "| " << other.line(note.range.bline + 1) << "\n";
                    }
                }

                for (diagnostic::fixman fixman : diag.fixes()) {
                    oss << impl::color::white << fixman.range.filename << impl::color::reset << ":" << impl::color::white << fixman.range.bline << impl::color::reset << ":" << impl::color::white << fixman.range.bcolumn << impl::color::reset << ": "  << color::green << "hint" << color::reset << ": " << message(fixman.hint) << '\n';

                    std::ostringstream tmp;

                    tmp << "  " << fixman.range.bline << std::string(std::to_string(file.lines_count()).size() - std::to_string(fixman.range.bline).size() + 1, ' ');
                    
                    std::string prefix1 = tmp.str();
                    std::string prefix2(prefix1.size(), ' ');
                    
                    justify(message(fixman.hint), oss, max_width);
                    oss << prefix1 << "| " << fix(file.line(fixman.range.bline), fixman) << "\n";
                }
            }

#if defined(__NEMESIS_DIAGNOSTIC_EXPLANATION__) && __NEMESIS_DIAGNOSTIC_EXPLANATION__
            if (!diag.explanation().empty()) justify(message(diag.explanation()), oss, max_width);
#endif

            return oss.str();
        }
    }
    
    void diagnostic_printer::handle(diagnostic diag)
    {
        stream_ << impl::nice(diag);
    }
}