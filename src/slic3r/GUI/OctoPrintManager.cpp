#include "OctoPrintManager.hpp"
#include <curl/curl.h>
#include <json/json.h>
#include <fstream>
#include <sstream>
#include <iostream>

namespace Slic3r {
namespace GUI {

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

static size_t ProgressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    if (dltotal > 0) {
        int percent = static_cast<int>((dlnow / dltotal) * 100);
        auto* cb = static_cast<OctoPrintManager::ProgressCallback*>(clientp);
        if (*cb) (*cb)(percent);
    }
    return 0;
}

class OctoPrintManager::Impl {
public:
    bool test_connection(const std::string& host, int port, const std::string& api_key) {
        std::string url = "http://" + host + ":" + std::to_string(port) + "/api/version";
        
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        std::string read_buffer;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-Api-Key: " + api_key).c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        return (res == CURLE_OK && !read_buffer.empty());
    }

    bool upload_file(const std::string& filepath, const OctoPrintConfig& config, 
                     ProgressCallback progress_cb, ErrorCallback error_cb, SuccessCallback success_cb) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        std::string url = "http://" + config.host + ":" + std::to_string(config.port) + "/api/files/local";
        
        // Preparar el formulario multipart
        curl_mime* form = curl_mime_init(curl);
        curl_mimepart* field = curl_mime_addpart(form);
        curl_mime_name(field, "file");
        curl_mime_filedata(field, filepath.c_str());
        
        if (!config.folder.empty()) {
            field = curl_mime_addpart(form);
            curl_mime_name(field, "folder");
            curl_mime_data(field, config.folder.c_str(), CURL_ZERO_TERMINATED);
        }

        if (config.start_print) {
            field = curl_mime_addpart(form);
            curl_mime_name(field, "select");
            curl_mime_data(field, "true", CURL_ZERO_TERMINATED);
            
            field = curl_mime_addpart(form);
            curl_mime_name(field, "print");
            curl_mime_data(field, "true", CURL_ZERO_TERMINATED);
        }

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("X-Api-Key: " + config.api_key).c_str());

        std::string read_buffer;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_MIME, form);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        
        if (progress_cb) {
            curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, ProgressCallback);
            curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &progress_cb);
        }

        CURLcode res = curl_easy_perform(curl);
        
        curl_easy_cleanup(curl);
        curl_mime_free(form);
        curl_slist_free_all(headers);

        if (res != CURLE_OK) {
            if (error_cb) error_cb("Upload failed: " + std::string(curl_easy_strerror(res)));
            return false;
        }

        // Parse JSON response
        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;
        std::istringstream stream(read_buffer);
        
        if (!Json::parseFromStream(builder, stream, &root, &errors)) {
            if (error_cb) error_cb("JSON parse error: " + errors);
            return false;
        }

        if (root.isMember("error")) {
            if (error_cb) error_cb(root["error"].asString());
            return false;
        }

        if (success_cb) success_cb();
        return true;
    }
};

OctoPrintManager::OctoPrintManager() : pImpl(std::make_unique<Impl>()) {}
OctoPrintManager::~OctoPrintManager() = default;

bool OctoPrintManager::test_connection(const std::string& host, int port, const std::string& api_key) {
    return pImpl->test_connection(host, port, api_key);
}

bool OctoPrintManager::upload_file(const std::string& filepath, const OctoPrintConfig& config, 
                                   ProgressCallback progress_cb, ErrorCallback error_cb, SuccessCallback success_cb) {
    return pImpl->upload_file(filepath, config, progress_cb, error_cb, success_cb);
}

bool OctoPrintManager::get_printer_state(PrinterState& state) {
    // Implementación básica - se puede expandir
    return false;
}

bool OctoPrintManager::start_print(const std::string& filename) {
    // Implementación básica - se puede expandir
    return false;
}

bool OctoPrintManager::cancel_print() {
    // Implementación básica - se puede expandir
    return false;
}

std::vector<std::string> OctoPrintManager::get_files() {
    // Implementación básica - se puede expandir
    return {};
}

bool OctoPrintManager::delete_file(const std::string& filename) {
    // Implementación básica - se puede expandir
    return false;
}

} // namespace GUI
} // namespace Slic3r
