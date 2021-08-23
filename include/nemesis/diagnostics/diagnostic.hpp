/**
 * @file diagnostic.hpp
 * @author Emanuel Buttaci
 * This file contains the fundamental unit for compiler diagnostics.
 * 
 */
#ifndef DIAGNOSTIC_HPP
#define DIAGNOSTIC_HPP

#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "nemesis/source/source.hpp"

namespace nemesis {
    class token;
}

std::ostream& operator<<(std::ostream& stream, nemesis::token t);

namespace nemesis {
    /**
     * Diagnostics class encapsulates a diagnostic error or warning message
     */
    class diagnostic {
        friend class builder;
    public:
        /**
         * Tag for diagnostics severity
         */
        enum class severity {
            /**
             * It's used for messages like usage or help message which don't require tags
             */
            none,
            /**
             * If any error occurs, compilation fails and no executable is generated
             */
            error, 
            /**
             * Warning is less strict than error, because executable can still be created
             */
            warning
        };
        /**
         * This struct represents a fix hint message
         * to help the user correcting a mistake in source code
         */
        struct fixman {
            /** 
             * Kind of action to perform in source text 
             */
            enum class action {
                /**
                 * Replaces a source range inside text
                 */
                replace,
                /**
                 * Insert new text in a source range inside text
                 */
                insert,
                /**
                 * Remove a source range from text
                 */
                remove
            };
            /**
             * Range of action
             */
            source_range range;
            /**
             * New text to insert in source
             */
            std::string fix;
            /**
             * Hint message
             */
            std::string hint;
            /**
             * Type of action
             */
            enum action action;
        };
        /**
         * This struct represents highlighted source text with hint message for main errors or cause
         */
        struct highlighter {
            /**
             * Highlighted source range
             */
            source_range range;
            /**
             * Hint message
             */
            std::string hint;
            /** 
             * Highlighting mode 
             */
            enum class mode { 
                /**
                 * Source text is coulored and marked with '^'
                 */
                heavy, 
                /**
                 * Source text is just coulored but not marked
                 */
                light 
            } mode;
        };
        class builder;
        /**
         * Format a message with arguments, like sprintf
         * @tparam Args Argument types
         * @param fmt Format string
         * @param args Arguments to format where placeholder '$' is found
         * @return Formatted string
         */
        template<typename... Args>
        static std::string format(const std::string& fmt, Args ...args);
        /**
         * @return Diagnostic severity
         */
        enum severity severity() const;
        /**
         * @return  Flag for small source description
         */
        bool small() const;
        /**
         * @return Location of source code that generated the diagnostic
         */
        source_location location() const;
        /**
         * @return Message associated to diagnostic
         */
        std::string message() const;
        /**
         * @return Explanation associated to diagnostic
         */
        std::string explanation() const;
        /**
         * @return Ranges of source code that will be underlined by '^' with message
         */
        std::vector<highlighter> highlighted() const;
        /**
         * @return Notes
         */
        std::vector<highlighter> notes() const;
        /**
         * @return List of fixs in source text
         */
        std::vector<fixman> fixes() const;
    private:
        /**
         * Diagnostic severity, none by default
         */
        enum severity severity_ = severity::none;
        /**
         * Tells if diagnostic description and source range is short
         */
        unsigned small_;
        /**
         * Location of source code that generated the diagnostic
         */
        source_location location_;
        /**
         * Error or warning or normal message associated to diagnostic
         */
        std::string message_;
        /**
         * Brief or long explanation associated to the diagnostic
         */
        std::string explanation_;
        /**
         * Ranges of source code that will be underlined by '^'
         * as they represent possible cause of errors with message
         */
        std::vector<highlighter> highlighted_;
        /**
         * Ranges of source code that will be underlined by '_'
         * for representings notes
         */
        std::vector<highlighter> notes_;
        /**
         * List of corrections in source text
         */
        std::vector<fixman> fixes_;
    };

    /**
     * This builder is a necessary tool to construct diagnostics
     */
    class diagnostic::builder {
        public:
            builder() = default;
            /**
            * Sets diagnostic severity
            * @return Reference to builder itself
            */
            builder& severity(enum severity severity);
            /**
            * Sets flag to print a small portion of source text
            * @return Reference to builder itself
            */
            builder& small(bool flag);
            /**
            * Sets diagnostic source location
            */
            builder& location(source_location location);
            /**
            * Adds a diagnostic message (error or warning or none)
            * @return Reference to builder itself
            */
            builder& message(std::string message);
            /**
            * Adds a diagnostic explanation (error or warning or none)
            * @return Reference to builder itself
            */
            builder& explanation(std::string message);
            /**
             * Adds a new highlitghted source range with inlined message
             * 
             * @param range Highlighted range
             * @param hint Hint message displayed next
             * @param mode Highlighting mode
             */
            builder& highlight(source_range range, std::string hint = {}, enum highlighter::mode mode = highlighter::mode::heavy);
            /**
             * Adds a new highlitghted source range without message
             * 
             * @param range Highlighted range
             * @param mode Highlighting mode
             */
            builder& highlight(source_range range, enum highlighter::mode mode);
            /**
             * Adds a new note with source range with inlined message
             * 
             * @param range Highlighted range
             * @param hint Message displayed next
             */
            builder& note(source_range range, std::string message = {});
             /**
             * Creates a replacement fix
             * @param range Range of action
             * @param fix New inserted text
             * @param message Hint message displayed to the user
             * @return Fixman object
             */
            builder& replacement(source_range range, std::string fix, std::string message);
            /**
             * Creates an insertion fix
             * @param range Range of action
             * @param fix New inserted text
             * @param message Hint message displayed to the user
             * @return Fixman object
             */
            builder& insertion(source_range range, std::string fix, std::string message);
            /**
             * Creates a removal fix
             * @param range Range of action
             * @param message Hint message displayed to the user
             * @return Fixman object
             */
            builder& removal(source_range range, std::string message);
            /**
            * @return Constructed diagnostic object
            */
            diagnostic build() const;
        private:
            /**
             * Diagnostic object to be built
             */
            diagnostic diag_;
    };

    /**
     * This abstract class defines a diagnostic subscriber, which is
     * the entity who registers for diagnostics and handle them
     */
    class diagnostic_subscriber {
    public:
        /**
         * Virtual method for handling diagnostics
         * 
         * @param diag Full constructed diagnostic delivered from publisher
         * @see diagnostic_publisher
         */
        virtual void handle(diagnostic diag) = 0;
    };

    /**
     * Diagnostic publisher is like the diagnostics server
     * which sends diagnostics to all subscribed clients (diagnostic_subscriber)
     * @see diagnostic_subscriber
     * @note It cannot be inherited
     */
    class diagnostic_publisher final {
    public:
        /**
         * Construct a new diagnostic publisher
         */
        diagnostic_publisher() = default;
        /**
         * The destructor publish error and waning
         * counters in a deffered manner
         */
        ~diagnostic_publisher();
        /**
         * Subscribes a new clinet
         * 
         * @param subscriber Reference to subscriber
         */
        void attach(diagnostic_subscriber& subscriber);
        /**
         * Unsubscribes a client
         * 
         * @param subscriber Reference to subscriber
         */
        void detach(diagnostic_subscriber& subscriber);
        /**
         * Sends the diagnostic to all subscribers
         * 
         * @param diag Diagnostic object
         */
        void publish(diagnostic diag);
        /**
         * @return Number of reported errors
         */
        unsigned errors() const;
        /**
         * @return Number of reported warnings
         */
        unsigned warnings() const;
    private:
        /**
         * Subscribers who will get diagnostics
         */
        std::set<diagnostic_subscriber*> subscribers_;
        /**
         * Error counter
         */
        unsigned errors_ = 0;
        /**
         * Warning counter
         */
        unsigned warnings_ = 0;
    };

    /**
     * Diagnostic printer is the main subscriber used in Nemesis
     * because it is designed to print well-formatted messages
     */
    class diagnostic_printer : public diagnostic_subscriber {
    public:
        diagnostic_printer() = delete;
        /**
         * Construct a new diagnostic printer associated to a file stream
         * 
         * @param stream Output file stream which will receive diagnostics
         */
        diagnostic_printer(std::ostream& stream);
        /**
         * Creates a well-formatted message from the diagnostics and
         * prints it to the output stream
         * 
         * @param diag Diagnostic object 
         */
        void handle(diagnostic diag) override;
    private:
        /**
         * Output stream on which diagnostic messages will be printed
         */
        std::ostream& stream_;
    };

    namespace impl {
        namespace color {
#if defined(__NEMESIS_COLORIZE__) && __NEMESIS_COLORIZE__ && (defined(__unix__) || defined(__linux__) || (defined(__APPLE__) && defined(__MACH__)))
            constexpr char reset[] = "\x1b[0m";
            constexpr char black[] = "\x1b[1;30m";
            constexpr char red[] = "\x1b[1;31m";
            constexpr char green[] = "\x1b[1;32m";
            constexpr char yellow[] = "\x1b[1;33m";
            constexpr char blue[] = "\x1b[1;34m";
            constexpr char magenta[] = "\x1b[1;35m";
            constexpr char cyan[] = "\x1b[1;36m";
            constexpr char white[] = "\x1b[1;37m";
            constexpr char lblack[] = "\x1b[0;30m";
            constexpr char lred[] = "\x1b[0;31m";
            constexpr char lgreen[] = "\x1b[0;32m";
            constexpr char lyellow[] = "\x1b[0;33m";
            constexpr char lblue[] = "\x1b[0;34m";
            constexpr char lmagenta[] = "\x1b[0;35m";
            constexpr char lcyan[] = "\x1b[0;36m";
            constexpr char lwhite[] = "\x1b[0;37m";
#else
            constexpr char reset[] = "";
            constexpr char black[] = "";
            constexpr char red[] = "";
            constexpr char green[] = "";
            constexpr char yellow[] = "";
            constexpr char blue[] = "";
            constexpr char magenta[] = "";
            constexpr char cyan[] = "";
            constexpr char white[] = "";
            constexpr char lblack[] = "";
            constexpr char lred[] = "";
            constexpr char lgreen[] = "";
            constexpr char lyellow[] = "";
            constexpr char lblue[] = "";
            constexpr char lmagenta[] = "";
            constexpr char lcyan[] = "";
            constexpr char lwhite[] = "";
#endif
        }

        template<typename... Args>
        void format(std::ostringstream& os, const char *fmt)
        {
            os << fmt;
        }

        template<typename Arg, typename... Args>
        void format(std::ostringstream& os, const char *fmt, Arg arg, Args ...args)
        {
            while (*fmt != '\0' && *fmt != '$') os << *(fmt++);
            if (*fmt == '$') {
                if (*(++fmt) == '{') {
                    switch (*(++fmt)) {
                        case 'r': os << color::red << arg << color::reset; break;
                        case 'g': os << color::green << arg << color::reset; break;
                        case 'y': os << color::yellow << arg << color::reset; break;
                        case 'b': os << color::blue << arg << color::reset; break;
                        case 'm': os << color::magenta << arg << color::reset; break;
                        case 'c': os << color::cyan << arg << color::reset; break;
                        case 'w': os << color::white << arg << color::reset; break;
                        case 'x': os << std::hex << arg << color::reset; break;
                        default: 
                            throw std::invalid_argument("impl::format(): invalid argument between `{}`");
                    }
                    if (*(++fmt) != '}') throw std::invalid_argument("impl::format(): missing closing `}`");
                    ++fmt;
                }
                else {
                    os << arg;
                }
                format(os, fmt, args...);
            }
        }

        template<typename Arg>
        void format(std::ostringstream& os, const char *fmt, Arg arg)
        {
            while (*fmt != '\0' && *fmt != '$') os << *(fmt++);
            if (*fmt == '$') {
                if (*(++fmt) == '{') {
                    switch (*(++fmt)) {
                        case 'r': os << color::red << arg << color::reset; break;
                        case 'g': os << color::green << arg << color::reset; break;
                        case 'y': os << color::yellow << arg << color::reset; break;
                        case 'b': os << color::blue << arg << color::reset; break;
                        case 'm': os << color::magenta << arg << color::reset; break;
                        case 'c': os << color::cyan << arg << color::reset; break;
                        case 'w': os << color::white << arg << color::reset; break;
                        case 'x': os << std::hex << arg << color::reset; break;
                        default: 
                            throw std::invalid_argument("impl::format(): invalid argument between `{}`");
                    }
                    if (*(++fmt) != '}') throw std::invalid_argument("impl::format(): missing closing `}`");
                    ++fmt;
                }
                else {
                    os << arg;
                }
                format(os, fmt);
            }
        }
    }

    template<typename... Args>
    std::string diagnostic::format(const std::string& fmt, Args ...args)
    {
        std::ostringstream output;
        impl::format(output, fmt.data(), args...);
        return output.str();
    }
}

#endif // DIAGNOSTIC_HPP