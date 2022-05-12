#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "binaryhandler.hpp"
#include "httplib.h"
#include <iostream>
#include "json.hpp"
#include "MimeTypes.hpp"
#include <string>
#pragma comment(lib, "libssl")
#pragma comment(lib, "libcrypto")

std::string file, token, mime, pass;
bool verb = false;

void error(std::string err_arg) {
    std::cerr << "Invalid Argument passed: " << err_arg << "\n";
    std::exit(1);
}

std::string get_best_server() {
    httplib::Client c("https://api.gofile.io");

    auto res = c.Get("/getServer");

    if (res) {
        std::string pre;

        try {
            nlohmann::json parsed = nlohmann::json::parse(res->body);

            if (parsed["status"] != "ok") {
                std::cerr << "Error at getting best server, dumped JSON: " << parsed.dump(4) << "\n";
                std::exit(1);
            }

            pre = parsed["data"]["server"];

            return pre;
        }
        catch (...) {
            std::cerr << "Error at parsing JSON while getting the best server\n";
            std::exit(1);
        }

    }
    else {
        std::cerr << "Failure at requesting best server\n";
        std::exit(1);
    }
}

int submit(std::string f, std::string t, std::string p) {
    if (verb) {
        std::cout << "Obtaining Best Server...\n";
    }

    std::string fbuf, server = get_best_server();

    if (verb) {
        std::cout << "Server selected: " << server << "\n";
    }

    try {
        nk125::binary_file_handler b;
        fbuf = b.read_file(f);
    }
    catch (std::exception& e) {
        std::cerr << "Error at reading file: " << e.what() << "\n";
        return 1;
    }

    if (verb) {
        std::cout << "Read File\n";
    }

    using namespace httplib;

    Client c("https://" + server + ".gofile.io");

    nlohmann::json res_json;
    
    try {
        if (mime.empty()) {
            mime = nk125::MimeTypes::getType(file);
        }
    }
    catch (std::exception& e) {
        std::cerr << "Error at finding mimetype: " << e.what() << "\n";
        return 1;
    }
    
    MultipartFormDataItems file_multipart = {
        {"file", fbuf, f, mime}
    };

    if (!p.empty()) {
        file_multipart.push_back(
            {"password", p, "", ""}
        );
    }
    
    if (!t.empty()) {
        file_multipart.push_back(
            {"token", t, "", ""}
        );
    }

    auto is_empty_str = [](std::string str) -> std::string {
        return (str.empty() ? "Empty" : str);
    };

    if (verb) {
        std::cout << "File: " << f << ", Mime type: " << mime << ", Token: " << is_empty_str(t) << ", Password: " << is_empty_str(p) << "\n";
    }

    Headers h = {
        {"User-Agent", "GoFile-CPP-Client/1.0"}
    };

    Result res = c.Post("/uploadFile", h, file_multipart);

    if (res) {
        try {
            res_json = nlohmann::json::parse(res->body);
        }
        catch (...) {
            std::cout << "Error at parsing JSON\n";
            return 1;
        }

        if (res_json["status"] == "ok") {
            std::cout << "File Submitted, Info: \n" << "Download URL: " << res_json["data"]["downloadPage"] << "\nFilename: " << res_json["data"]["fileName"] << "\nMD5 Hash: " << res_json["data"]["md5"] << "\nDirect: " << res_json["data"]["directLink"] << "\n";
            return 0;
        }
        else {
            std::cerr << "Something failed, dumped JSON response: " << res_json.dump(4) << "\n";
            return 1;
        }
    }
    else {
        std::cerr << "Failure at requesting\n";
        return 1;
    }
}

void parse(int argc, char* argv[]) {
    if (argc <= 1) {
        std::cout <<    "Usage:\n  -f = File"
                        "\n  -m = Override mimetype detection (Optional, example: -m text/plain)"
                        "\n  -t = API Token (Optional)"
                        "\n  -p = Password (Optional)"
                        "\n  -v = Verbose";
        std::exit(0);
    }
    else {
        for (int i = 1; i <= argc - 1; i++) {
            std::string c_arg = argv[i];
            bool not_next = i + 1 > argc - 1;

            switch (c_arg[1]) {
                case 'f':
                    if (not_next) {
                        std::cout << "File isn't defined!\n";
                        std::exit(0);
                    }

                    file = argv[i + 1];
                    i++;
                    break;
                case 'm':
                    if (not_next) {
                        std::cout << "Mimetype isn't defined!\n";
                        std::exit(0);
                    }

                    mime = argv[i + 1];
                    i++;
                    break;
                case 't':
                    if (not_next) {
                        token = "";
                    }
                    else {
                        token = argv[i + 1];
                    }

                    i++;
                    break;
                case 'v':
                    verb = true;
                    break;
                case 'p':
                    if (not_next) {
                        pass = "";
                    }
                    else {
                        pass = argv[i + 1];
                    }

                    i++;
                    break;
                default:
                    error(c_arg);
                    break;
            }
        }
    }

    if (file.empty()) {
        std::cerr << "File don't defined\n";
        std::exit(1);
    }
}

int main(int argc, char* argv[]) {
    parse(argc, argv);

    return submit(file, token, pass);
}
