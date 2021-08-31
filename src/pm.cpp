#include <fstream>
#include <sstream>

#include <zip.h>
#include <curl/curl.h>

#include "nemesis/diagnostics/diagnostic.hpp"
#include "nemesis/pm/pm.hpp"

namespace nemesis {
    namespace pm {
        int compare_version(std::string a, std::string b)
        {
            // we assume versions are correct
            std::istringstream astream(a), bstream(b);
            int amajor, aminor, apatch;
            int bmajor, bminor, bpatch;
            // info
            std::string ainfo, binfo;
            // majors
            std::getline(astream, ainfo, '.');
            std::getline(bstream, binfo, '.');
            amajor = std::stoi(ainfo);
            bmajor = std::stoi(binfo);
            // minors
            std::getline(astream, ainfo, '.');
            std::getline(bstream, binfo, '.');
            aminor = std::stoi(ainfo);
            bminor = std::stoi(binfo);
            // patches
            std::getline(astream, ainfo, '.');
            std::getline(bstream, binfo, '.');
            apatch = std::stoi(ainfo);
            bpatch = std::stoi(binfo);
            // compare
            if (amajor > bmajor) return 1;
            if (amajor < bmajor) return -1;
            if (aminor > bminor) return 1;
            if (aminor < bminor) return -1;
            if (apatch > bpatch) return 1;
            if (apatch < bpatch) return -1;
            return 0;
        }

        manifest manager::parse_manifest_file_from_buffer(std::istream& stream) const
        {
            manifest result;
            // input words
            std::string line, key, value, section;
            // reads lines
            while (std::getline(stream, line)) {
                if (line == "@application") {
                    // if package kind was already set, then we have an error
                    if (result.kind != manifest::kind::none) error("You have already declared package as `$`, you cannot redefine it, idiot!", result.kind == manifest::kind::app ? "application" : "library");
                    // sets package kind as application
                    result.kind = manifest::kind::app;
                    // sets current section
                    section = line;
                }
                else if (line == "@library") {
                    // if package kind was already set, then we have an error
                    if (result.kind != manifest::kind::none) error("You have already declared package as `$`, you cannot redefine it, idiot!", result.kind == manifest::kind::app ? "application" : "library");
                    // sets package kind as library
                    result.kind = manifest::kind::lib;
                    // sets current section
                    section = line;
                }
                else if (line == "@dependencies") {
                    // if dependency block was already defined, then error
                    if (!result.dependencies.empty()) error("You are duplicating dependencies' blocks, idiot!");
                    // sets current section
                    section = line;
                }
                // comment
                else if (line.front() == '#') continue;
                // properties declared outside of a section
                else if (section.empty()) error("You must declare a section like `@application`, `@library` or `@dependencies` before properties, c*nt!");
                else {
                    std::istringstream linestream(line);
                    // inside application section
                    if (section == "@application") {
                        // now parse key-value pairs
                        while (linestream >> key) {
                            if (!(linestream >> value)) error("You forgot to specify value for property `$`, dumb*ss!", key);
                            // comment
                            else if (key.front() == '#') continue;
                            else if (key == "name") {
                                // remove quotes from string
                                if (!is_valid_string_value(value)) error("`$` is not a valid string value!", value);
                                else value = value.substr(1, value.size() - 2);
                                if (!is_valid_package_name(value)) error("`$` is not a valid application package name!", value);
                                else result.name = value;
                            }
                            else if (key == "version") {
                                // remove quotes from string
                                if (!is_valid_string_value(value)) error("`$` is not a valid string value!", value);
                                else value = value.substr(1, value.size() - 2);
                                if (!is_valid_package_version(value)) error("`$` is not a valid package version!", value);
                                else result.version = value;
                            }
                            else if (key == "builtin") {
                                if (value == "true") result.builtin = true;
                                else if (value == "false") result.builtin = false;
                                else error("`$` is not a value for `builtin` property, which can be `true` or `false`!", value);
                            }
                            else error("`$` is not a valid property for manifest file!", key);
                        }
                    }
                    // inside library section
                    else if (section == "@library") {
                        // now parse key-value pairs
                        while (linestream >> key) {
                            if (!(linestream >> value)) error("You forgot to specify value for property `$`, dumb*ss!", key);
                            // comment
                            else if (key.front() == '#') continue;
                            else if (key == "name") {
                                // remove quotes from string
                                if (!is_valid_string_value(value)) error("`$` is not a valid string value!", value);
                                else value = value.substr(1, value.size() - 2);
                                if (!is_valid_package_name(value)) error("`$` is not a valid library package name!", value);
                                else result.name = value;
                            }
                            else if (key == "version") {
                                // remove quotes from string
                                if (!is_valid_string_value(value)) error("`$` is not a valid string value!", value);
                                else value = value.substr(1, value.size() - 2);
                                if (!is_valid_package_version(value)) error("`$` is not a valid package version!", value);
                                else result.version = value;
                            }
                            else if (key == "builtin") {
                                if (value == "true") result.builtin = true;
                                else if (value == "false") result.builtin = false;
                                else error("`$` is not a value for `builtin` property, which can be `true` or `false`!", value);
                            }
                            else error("`$` is not a valid property for manifest file!", key);
                        }
                    }
                    // inside dependencies section
                    else {
                        // now parse key-value pairs
                        while (linestream >> key) {
                            // comment
                            if (key.front() == '#') continue;
                            else if (result.dependencies.count(key) > 0) error("You are duplicating `$` dependency declaration, idiot!", key);
                            // version is specified
                            else if (linestream >> value) {
                                if (!is_valid_string_value(value)) error("`$` is not a valid string value!", value);
                                else value = value.substr(1, value.size() - 2);
                                if (!is_valid_package_version(value)) error("`$` is not a valid package version!", value);
                                else result.dependencies.emplace(key, package { key, value });
                            }
                            // version is not specified, so most stable is used
                            else result.dependencies.emplace(key, package { key, {} });
                        }
                    }
                }
            }
            // correct, return result
            return result;
        }

        manager::manager(diagnostic_publisher& publisher, source_handler& handler) : publisher_(publisher), source_handler_(handler) 
        {
            // initialize curl globally
            curl_global_init(CURL_GLOBAL_ALL);
        }

        manager::~manager()
        {
            // curl global clean up
            curl_global_cleanup();
        }

        manager& manager::instance(diagnostic_publisher& publisher, source_handler& handler)
        {
            static class manager manager(publisher, handler);
            return manager;
        }

        manifest manager::parse_manifest_file(std::string path) const
        {
            // simple approach for parsing
            std::ifstream stream(path.data(), std::ios_base::in);
            // checks if file is open
            if (!stream) error("I cannot open manifest file `$`, f*ck...", path);
            // parse from input buffer
            return parse_manifest_file_from_buffer(stream);
        }

        static bool parse_lock_info_from_line(std::string line, lock::info& info)
        {
            constexpr const char delimiter = ':';
            std::istringstream stream(line);
            std::string current;
            // package name
            if (!std::getline(stream, info.name, delimiter) || !is_valid_package_name(info.name)) return false;
            // package version
            if (!std::getline(stream, info.version, delimiter) || !is_valid_package_version(info.version)) return false;
            // package builtin
            if (!std::getline(stream, current, delimiter) || !is_valid_boolean_value(current)) return false;
            info.builtin = current == "true" ? true : false;
            // package hash
            if (!std::getline(stream, info.hash, delimiter) || !is_valid_hash_value(info.hash)) return false;
            // package local path
            if (!std::getline(stream, info.path, delimiter)) return false;
            // all went fine
            return true;
        }

        lock manager::parse_lock_file(std::string path) const
        {
            lock result;
            // simple approach for parsing
            std::ifstream stream(path.data(), std::ios_base::in);
            // checks if file is open
            if (!stream) error("I cannot open lock file `$`, f*ck...", path);
            // input words
            std::string line;
            // reads lines
            while (std::getline(stream, line)) {
                if (line == "@application" || line == "@library") {
                    // package kind
                    result.kind = line == "@library" ? manifest::kind::lib : manifest::kind::app;
                    // if package kind was already set, then we have an error
                    if (!result.package.name.empty()) error("You have already declared package `$` inside `nemesis.lock`, you cannot redefine it, idiot!", result.package.name);
                    // now parse current package line info
                    if (!std::getline(stream, line)) error("Damn, information for current package is missing after `$`, file `nemesis.lock` may be corrupted.", line);
                    // split line
                    if (!parse_lock_info_from_line(line, result.package)) error("Damn, information for current package is corrupted, you need to regenerate `nemesis.lock` file!");
                }
                else if (line == "@dependencies") {
                    // if dependency block was already defined, then error
                    if (!result.dependencies.empty()) error("You are duplicating dependencies' blocks inside `nemesis.lock`, idiot!");
                    // for duplicates
                    std::set<std::string> dependencies;
                    // parses all dependencies
                    for (lock::info info; std::getline(stream, line);) {
                        // split line
                        if (!parse_lock_info_from_line(line, info)) error("Damn, information for dependencies is corrupted, you need to regenerate `nemesis.lock` file!");
                        // test that dependency is not duplicated
                        if (dependencies.count(info.name)) error("You are duplicating `$` dependency inside `nemesis.lock`, idiot!", info.name);
                        // adds dependency
                        else {
                            dependencies.emplace(info.name);
                            result.dependencies.push_back(info);
                        }
                    }
                }
            }
            // checks no info is missing, even though `core` dependency is missing it is added forcefully later
            if (result.package.name.empty()) error("Information about current package is missing, dammit! File `nemesis.lock` may be corrupted.");
            // correct, return result
            return result;
        }

        void manager::dump_manifest_file(pm::manifest manifest, std::string path) const
        {
            std::ofstream outfile(path.data(), std::ios_base::trunc);
            // unable to open
            if (!outfile) error("Sorry, can't create manifest file `$`...", path);
            // prints package
            outfile << (manifest.kind == manifest::kind::app ? "@application\n" : "@library\n")
                    << "name '" << manifest.name << "'\n"
                    << "version '" << manifest.version << "'\n"
                    << "builtin " << (manifest.builtin ? "true" : "false") << "\n";
            // prints dependencies
            outfile << "@dependencies\n";
            for (auto dep : manifest.dependencies) {
                if (dep.second.version.empty()) outfile << dep.second.name << "\n";
                else outfile << dep.second.name << " '" << dep.second.version << "'\n";
            }
        }

        void manager::dump_lock_file(pm::lock lock, std::string path) const
        {
            std::ofstream outfile(path.data(), std::ios_base::trunc);
            // unable to open
            if (!outfile) error("Sorry, can't create lock file `$`...", path);
            // prints package
            outfile << (lock.kind == manifest::kind::app ? "@application\n" : "@library\n");
            outfile << lock.package.name << ':' << lock.package.version << ':' << (lock.package.builtin ? "true" : "false") << ':' << lock.package.hash << ':' << lock.package.path << '\n';
            // prints dependencies
            outfile << "@dependencies\n";
            for (auto dep : lock.dependencies) outfile << dep.name << ':' << dep.version << ':' << (dep.builtin ? "true" : "false") << ':' << dep.hash << ':' << dep.path << '\n';
        }
        
        lock manager::generate_lock_file(pm::manifest manifest, std::string where) try
        {
            lock result;
            // sets current package
            result.kind = manifest.kind;
            result.package.name = manifest.name;
            result.package.version = manifest.version;
            result.package.builtin = manifest.builtin;
            result.package.path = std::filesystem::current_path();
            result.package.hash = "deadbeef";
            // resolution of conflicts by downloading files
            auto graph = resolve(manifest);
            // constructs lock file from dependency tree using topological order, last node is package, so it's excluded
            auto dependency = graph.topological.begin();
            // iterates over dependencies excluding package itself
            for (std::size_t index = 0; index < graph.topological.size() - 1; ++index, ++dependency) result.dependencies.push_back(*dependency);
            // creates lock file on local package directory
            dump_lock_file(result, where);
            // yields lock file information
            return result;
        }
        catch (pm::exception&) {
            // error code for file system operations
            std::error_code code;
            // remove all cached files
            std::filesystem::remove_all(manager::cache_path, code);
            // throw again
            throw;
        }

        lock manager::add_dependency(pm::manifest manifest, std::string lockpath, std::string name, std::string version)
        {
            // old manifest is set to be restored on future failure
            restored_ = manifest;
            // new dependency is already installed
            if (manifest.dependencies.count(name) > 0) {
                // different scenarios
                // 1. new version is not specified, so we don't do anything
                if (version.empty()) return generate_lock_file(manifest, lockpath);
                // 2. version is specified
                int compare = compare_version(version, manifest.dependencies[name].version);
                // 2.a. new version is greater, so we do an upgrade
                if (compare > 0) warning("Tryna do upgrade of package `$` `$` -> `$`, let's see...", name, manifest.dependencies[name].version, version);
                // 2.b. new version is lower, so we do a downgrade
                else if (compare < 0) warning("Tryna do downgrade of package `$` `$` -> `$`, hope you're sure of this...", name, manifest.dependencies[name].version, version);
                // 2.c they're equal, so no action takes place
                // in all cases, new version is set
                manifest.dependencies[name].version = version;
            }
            // not yet installed, so it is added
            else {
                message("Adding package `$$` to your dependencies, brother...", version.empty() ? "" : " " + version, name);
                manifest.dependencies.emplace(name, pm::package { name, version });
            }
            // dumps manifest
            dump_manifest_file(manifest, manager::manifest_path);
            // generates and writes new lock file
            return generate_lock_file(manifest, lockpath);
        }
        
        lock manager::remove_dependency(pm::manifest manifest, std::string lockpath, std::string name)
        {
            // old manifest is set to be restored on future failure
            restored_ = manifest;
            // dependency is installed, so it needs to be removed
            if (manifest.dependencies.count(name) > 0) {
                message("Removing package `$` from your dependencies...", name);
                manifest.dependencies.erase(name);
            }
            // dependency is not installed, so error
            else error("What the hell you're doing? Package `$` is not installed here.", name);
            // dumps manifest
            dump_manifest_file(manifest, manager::manifest_path);
            // generates and writes new lock file
            return generate_lock_file(manifest, lockpath);
        }

        void manager::restore() const
        {
            // since manifest was set buy provious calls, it is restored
            if (restored_.kind != manifest::kind::none && !restored_.name.empty()) {
                // error code for file system operations
                std::error_code code;
                // writes back old manifest on disk
                dump_manifest_file(restored_, manager::manifest_path);
                // remove lock file
                std::filesystem::remove(manager::lock_path);
                // log
                message("Manifest file `$` restored, brother, easy.", manager::manifest_path);
            }
        }

        void manager::extract_package_archive(std::string archive, std::string to)
        {
            // error code for file system operations
            std::error_code code;
            // error code for archive operations
            int err = 0;
            // open zip archive
            zip* zarchive = zip_open(archive.data(), 0, &err);
            // check error
            if (!zarchive) error("I can't unzip archive `$`, dammit!", archive);
            // for each entry of archive
            for (std::size_t i = 0; i < zip_get_num_entries(zarchive, 0); ++i) {
                struct zip_stat stat;
                zip_stat_init(&stat);
                // retrieve info of entry at index <i>
                zip_stat_index(zarchive, i, 0, &stat);
                // length of path
                auto pathlen = std::strlen(stat.name);
                // creates directory
                if (pathlen > 0 && stat.name[pathlen - 1] == '/') {
                    // creates inner directories if they don't exist
                    std::filesystem::create_directories(to + "/" + stat.name, code);
                }
                // creates file
                else {
                    // open file stream
                    std::ofstream stream(to + "/" + stat.name);
                    // check
                    if (!stream) error("I have problems extracting dependency `$` into your local `$` directory, f*ck.", archive, to);
                    // allocates memory buffer for manifest file content
                    char* buffer = new char[stat.size];
                    // open compressed file
                    zip_file* zfile = zip_fopen(zarchive, stat.name, 0);
                    // check error
                    if (!zfile) {
                        delete[] buffer;
                        error("I can't unzip file `$` from archive `$`, dammit!", stat.name, archive);
                    }
                    // read its content inside the buffer
                    zip_fread(zfile, buffer, stat.size);
                    // closes zipped file
                    zip_fclose(zfile);
                    // now prints out content on file inside dependencies directory
                    stream.write(buffer, stat.size);
                    // deallocates memory of file content
                    delete[] buffer;
                }
            }
            // closes archive
            zip_close(zarchive);
        }

        manifest manager::unzip_package_manifest(std::string package, std::string path)
        {
            // error code for operations
            int err = 0;
            // open zip archive
            zip* archive = zip_open(path.data(), 0, &err);
            // check error
            if (!archive) error("I can't unzip archive `$`, dammit!", path);
            // manifest path inside archive
            std::string archive_manifest_path = package + "/" + manager::manifest_path;
            // search manifest `nemesis.manifest` inside package archive
            struct zip_stat stat;
            zip_stat_init(&stat);
            zip_stat(archive, archive_manifest_path.data(), 0, &stat);
            // allocates memory buffer for manifest file content
            char* buffer = new char[stat.size + 1] {0};
            // open compressed file
            zip_file* file = zip_fopen(archive, archive_manifest_path.data(), 0);
            // check error
            if (!file) {
                delete[] buffer;
                error("I can't unzip file `$` from archive `$`, dammit!", archive_manifest_path, path);
            }
            // read its content inside the buffer
            zip_fread(file, buffer, stat.size);
            // closes file
            zip_fclose(file);
            // closes archive
            zip_close(archive);
            // constructs input stream
            std::istringstream stream(std::string(buffer, stat.size));
            // deallocates memory
            delete[] buffer;
            // now parses manifest file
            return parse_manifest_file_from_buffer(stream);
        }

        // this callback is used for writing data from http request to downloaded file
        static std::size_t receive(void* ptr, std::size_t size, std::size_t n, void* stream) { return fwrite(ptr, size, n, reinterpret_cast<std::FILE*>(stream)); }

        std::list<pm::package> manager::download_package(pm::package& package, pm::lock::info& info)
        {
            // central package registry server
            const std::string server = "http://localhost:8000";
            // destination for downloaded zip file
            const std::string destination = std::string(manager::cache_path) + "/" + package.name + ".zip";
            // error code for file system operations
            std::error_code code;
            // info code of http operation
            long status;
            // handle used for operations
            CURL* handle = curl_easy_init();
            // set up url for GET request to download package without version
            if (package.version.empty()) {
                message("Downloading package `$`...", package.name);
                curl_easy_setopt(handle, CURLOPT_URL, diagnostic::format("$/?package=$", server, package.name).data());
            }
            // set up url for GET request to download package with specific version
            else {
                message("Downloading package `$ $`...", package.name, package.version);
                curl_easy_setopt(handle, CURLOPT_URL, diagnostic::format("$/?package=$&version=$", server, package.name, package.version).data());
            }
            // set write function to add data to file stream
            curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, receive);
            // open file for output
            std::FILE* output = std::fopen(destination.data(), "w");
            // if we can't open file, then error
            if (!output) error("I can't create file `$` while downloading package `$` from central registry, goddamn!", destination, package.name);
            // set file output
            curl_easy_setopt(handle, CURLOPT_WRITEDATA, output);
            // make http GET request to download package
            curl_easy_perform(handle);
            // get information about result
            curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &status);
            // close file
            std::fclose(output);
            // operation failure
            if (status != 200) error("I had trouble downloading package `$` from central registry, maybe it doesn't exist...", package.name);
            // operations success
            message("Package `$$` downloaded!", package.name, package.version.empty() ? "" : " " + package.version);
            // unzip its manifest to get information
            pm::manifest manifest = unzip_package_manifest(package.name, destination);
            // constructs dependencies
            std::list<pm::package> dependencies;
            // each one is added
            for (auto entry : manifest.dependencies) dependencies.push_back(entry.second);
            // fill information about current package from its manifest
            info.name = manifest.name;
            info.version = manifest.version;
            info.builtin = manifest.builtin;
            info.hash = "deadbeef";
            info.path = std::string(manager::dependencies_path) + "/" + manifest.name;
            // dependencies of current package
            return dependencies;
        }
        
        void manager::dfs(dependency_graph& graph, std::set<std::string>& visited, package current)
        {
            // sets current node as visited
            visited.insert(current.name);
            // info abount current package
            pm::lock::info info;
            // downloads package dependencies from central repository
            std::list<pm::package> dependencies = download_package(current, info);
            // for each dependency
            for (auto dependency : dependencies) {
                if (visited.count(dependency.name) > 0) {
                    // cyclic dependency, visited but not yet resolved
                    if (graph.nodes.count(dependency.name) == 0) error("Cyclic dependency with package `$`!", dependency.name);
                    // dependency conflict, two different versions, the algorithm chooses the lowest one
                    //else if (!dependency.version.empty() && graph.nodes[dependency.name].package.version != dependency.version) error("Dependency conflict, package `$` appears in both versions `$` and `$`!", dependency.name, graph.nodes[dependency.name].package.version, dependency.version);
                    // dependency conflict, two different versions, the algorithm chooses the lowest one
                    else if (!dependency.version.empty() && !graph.nodes[dependency.name].package.version.empty()) {
                        // compare version of new dependency against already resolved dependency's version
                        int compare = compare_version(graph.nodes[dependency.name].package.version, dependency.version);
                        // this means that dependency (lowest) is already downloaded as it is the resolved one
                        if (compare < 0) warning("I found dependency `$` duplication between versions `$` and `$`, choosing `$`, okay?", dependency.name, graph.nodes[dependency.name].package.version, dependency.version, graph.nodes[dependency.name].package.version);
                        // this means that dependency is not yet downloaded as we selected the new dependency's version (lowest)
                        else if (compare > 0) {
                            warning("I found dependency `$` duplication between versions `$` and `$`, choosing `$`, okay?", dependency.name, graph.nodes[dependency.name].package.version, dependency.version, graph.nodes[dependency.name].package.version);
                            // remove previous dependency (unset resolved)
                            graph.nodes.erase(dependency.name);
                            // (unset visited)
                            visited.erase(dependency.name);
                            // erase it from topological order
                            std::remove_if(graph.topological.begin(), graph.topological.end(), [&] (pm::lock::info package) { return dependency.name == package.name; });
                            // traverse new dependency's version
                            dfs(graph, visited, dependency);
                        }
                        // in case they're equal or the new dependency's version is not specified, no action takes place as we consider the resolved dependency the right one
                    }
                }
                // if not yet visited, then we visit it
                else dfs(graph, visited, dependency);
            }
            // in the end current package has been resolved, after all its dependencies
            graph.nodes[current.name] = dependency_graph::node { current, dependencies };
            // adds node for topological sort since it's been resolved
            graph.topological.push_back(info);
        }

        dependency_graph manager::resolve(pm::manifest manifest)
        {
            // error code for file system operations
            std::error_code code;
            // clears old dependencies
            std::filesystem::remove_all(manager::dependencies_path, code);
            // recreates the dependencies folder empty
            std::filesystem::create_directory(manager::dependencies_path, code);
            // creates cache for downloaded libraries
            std::filesystem::create_directory(manager::cache_path, code);
            // result graph
            dependency_graph graph;
            // List of visited nodes
            std::set<std::string> visited;
            // source node
            dependency_graph::node source { package { manifest.name, manifest.version }, {} };
            for (auto dependency : manifest.dependencies) source.edges.push_back(dependency.second);
            // sets source as visited but not resolved
            visited.insert(manifest.name);
            // depth first search with package fetching
            for (auto dependency : source.edges) {
                if (visited.count(dependency.name) > 0) {
                    // cyclic dependency, visited but not yet resolved
                    if (graph.nodes.count(dependency.name) == 0) error("Cyclic dependency with package `$`!", dependency.name);
                    // dependency conflict, two different versions, the algorithm chooses the lowest one
                    else if (!dependency.version.empty() && !graph.nodes[dependency.name].package.version.empty()) {
                        // compare version of new dependency against already resolved dependency's version
                        int compare = compare_version(graph.nodes[dependency.name].package.version, dependency.version);
                        // this means that dependency (lowest) is already downloaded as it is the resolved one
                        if (compare < 0) warning("I found dependency `$` duplication between versions `$` and `$`, choosing `$`, okay?", dependency.name, graph.nodes[dependency.name].package.version, dependency.version, graph.nodes[dependency.name].package.version);
                        // this means that dependency is not yet downloaded as we selected the new dependency's version (lowest)
                        else if (compare > 0) {
                            warning("I found dependency `$` duplication between versions `$` and `$`, choosing `$`, okay?", dependency.name, graph.nodes[dependency.name].package.version, dependency.version, graph.nodes[dependency.name].package.version);
                            // remove previous dependency (unset resolved)
                            graph.nodes.erase(dependency.name);
                            // (unset visited)
                            visited.erase(dependency.name);
                            // erase it from topological order
                            std::remove_if(graph.topological.begin(), graph.topological.end(), [&] (pm::lock::info package) { return dependency.name == package.name; });
                            // traverse new dependency's version
                            dfs(graph, visited, dependency);
                        }
                        // in case they're equal or the new dependency's version is not specified, no action takes place as we consider the resolved dependency the right one
                    }
                }
                // if not yet visited, then we visit it
                else dfs(graph, visited, dependency);
            }
            // puts our package as source vertex
            graph.nodes[manifest.name] = source;
            // dumps packages in topological order onto lock file and extracts them into `libs` directory (dependencies)
            for (auto package : graph.topological) {
                // extracts dependency archive (for example `math.zip`) inside dependency directory (so in `libs`)
                extract_package_archive(std::string(manager::cache_path) + "/" + package.name + ".zip", manager::dependencies_path);
            }
            // source goes last in this case as it's been resolved for last
            graph.topological.push_back(pm::lock::info { manifest.name, manifest.version, manifest.builtin, "deadbeef", std::filesystem::current_path() });
            // remove cache of downloaded libraries
            std::filesystem::remove_all(manager::cache_path, code);
            // yields result
            return graph;
        }

        void manager::load_package_workspace(compilation& compilation, pm::lock::info package, pm::lock lockfile, bool is_dependency) const
        {
            // construct all sources for this workspace
            compilation::sources sources, cpp_sources;
            // traverse sources inside 'src' directory
            for (auto entry : std::filesystem::directory_iterator(package.path + "/src")) {
                auto path = utf8::span::builder().concat(entry.path().string().data()).build();
                if (!source_handler_.load(path)) error("I had some problems opening file `$`, I have to stop here, I am sorry...", path);
                else sources.push_back(&source_handler_.get(path));
            }
            // traverse sources inside 'cpp' directory to get cpp sources
            for (auto entry : std::filesystem::directory_iterator(package.path + "/cpp")) {
                auto path = utf8::span::builder().concat(entry.path().string().data()).build();
                if (!source_handler_.load(path)) error("I had some problems opening file `$`, I have to stop here, I am sorry...", path);
                else {
                    source_file& cppsource = source_handler_.get(path);
                    cpp_sources.push_back(&cppsource);
                }
            }
            // if dependency, then it is added to our compilation
            if (is_dependency) compilation.dependency(package.name, package.version, sources, cpp_sources, package.builtin);
            // current workspace
            else compilation.current(package.name, package.version, sources, cpp_sources, package.builtin, lockfile.kind == manifest::kind::app ? compilation::package::kind::app : lockfile.kind == manifest::kind::lib ? compilation::package::kind::lib : compilation::package::kind::none);
        }

        compilation manager::build_compilation_chain(pm::lock lockfile) const
        {
            // construct compilation chain
            compilation compilation(publisher_, source_handler_);
            // loads core library package, which is the only depedency that is not directly moved into `libs` directory of current package only if
            // i. current package is not `core` itself
            // ii. current package does not list `core` among its dependencies
            if (lockfile.package.name != "core" && std::count_if(lockfile.dependencies.begin(), lockfile.dependencies.end(), [] (pm::lock::info dep) { return dep.name == "core"; }) == 0) load_core_library(compilation);
            // adds package dependencies
            for (auto dependency : lockfile.dependencies) load_package_workspace(compilation, dependency, lockfile, true);
            // last one is current package workspace
            load_package_workspace(compilation, lockfile.package, lockfile, false);
            // constructed compilation object
            return compilation;
        }

        void manager::load_core_library(compilation& compilation) const
        {
            // loads file into cache
            auto cppheader = utf8::span::builder().concat(std::string(std::getenv("HOME")).append("/Desktop/nemesis/libcore/cpp/core.h").data()).build();
            if (!source_handler_.load(cppheader)) error("I'm not able to load `core` library from installation directory, dammit!");
            auto cppsource = utf8::span::builder().concat(std::string(std::getenv("HOME")).append("/Desktop/nemesis/libcore/cpp/core.cpp").data()).build();
            if (!source_handler_.load(cppsource)) error("I'm not able to load `core` library from installation directory, dammit!");
            auto source = utf8::span::builder().concat(std::string(std::getenv("HOME")).append("/Desktop/nemesis/libcore/src/core.ns").data()).build();
            if (!source_handler_.load(source)) error("I'm not able to load `core` library from installation directory, dammit!");
            // loads core manifest for info
            auto manifest = parse_manifest_file(std::string(std::getenv("HOME")).append("/Desktop/nemesis/libcore/nemesis.manifest"));
            // creates the core package
            compilation.dependency(manifest.name, manifest.version, { &source_handler_.get(source) }, { &source_handler_.get(cppheader), &source_handler_.get(cppsource) }, manifest.builtin);
        }
    }
}