#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <algorithm>

using json = nlohmann::json;
using std::string, std::vector, std::cout, std::endl, std::find;



static size_t write_to_string(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = static_cast<std::string*>(userdata);
    size_t count = size * nmemb;
    out->append(static_cast<char*>(ptr), count);
    return count;
}

std::string http_post(const std::string& url, const std::string& fields) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");

    std::string body;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "cpp-libcurl/grades/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, ""); // allow gzip/deflate
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode rc = curl_easy_perform(curl);
    if (rc != CURLE_OK) {
        std::string msg = curl_easy_strerror(rc);
        curl_easy_cleanup(curl);
        throw std::runtime_error("HTTP error: " + msg);
    }
    curl_easy_cleanup(curl);
    return body;
}

int to_int(const json& j, const char* key) {
    if (!j.contains(key)) return 0;
    if (j[key].is_number_integer()) return j[key].get<int>();
    if (j[key].is_string()) return std::stoi(std::string(j[key]));
    return 0;
}
double to_double(const json& j, const char* key) {
    if (!j.contains(key)) return 0.0;
    if (j[key].is_number()) return j[key].get<double>();
    if (j[key].is_string()) return std::stod(std::string(j[key]));
    return 0.0;
}


// string getClassData (string course[2], int argc, char** argv) {
string getClassData (string deptAbbr, int num, int argc, char** argv) { //processes anex.us data
    string info = "";

    // std::cout << "num" << std::endl;

    try {
        // std::string dept   = (argc > 1) ? argv[1] : "CSCE";
        // std::string number = (argc > 2) ? argv[2] : "120";
        // std::string dept   = (argc > 1) ? argv[1] : course[0];
        // std::string number = (argc > 2) ? argv[2] : course[1];
        std::string dept   = (argc > 1) ? argv[1] : deptAbbr;
        std::string number = (argc > 2) ? argv[2] : std::to_string(num);

        curl_global_init(CURL_GLOBAL_DEFAULT);

        const std::string url = "https://anex.us/grades/getData/";
        std::string fields = "dept=" + dept + "&number=" + number;

        std::string response = http_post(url, fields);
        json j = json::parse(response);

        if (!j.contains("classes") || !j["classes"].is_array()) {
            std::cerr << "Unexpected JSON shape, first 2KB:\n"
                      << response.substr(0, 2000) << "\n";
            curl_global_cleanup();
            return "invalid 2";
        }

        const json& rows = j["classes"];

        std::cout << "Year,Semester,Prof,Section,GPA,A,B,C,D,F,I,Q,S,U,X\n";
        for (const auto& r : rows) {
            int    year     = to_int(r, "year");
            std::string sem = r.value("semester", "");
            std::string prof= r.value("prof", "");
            int    section  = to_int(r, "section");
            double gpa      = to_double(r, "gpa");

            int A = to_int(r,"A"), B = to_int(r,"B"), C = to_int(r,"C"),
                D = to_int(r,"D"), F = to_int(r,"F"), I = to_int(r,"I"),
                Q = to_int(r,"Q"), S = to_int(r,"S"), U = to_int(r,"U"),
                X = to_int(r,"X");

            std::cout << year << ',' << sem << ',' << prof << ','
                      << section << ',' << gpa << ','
                      << A << ',' << B << ',' << C << ','
                      << D << ',' << F << ',' << I << ','
                      << Q << ',' << S << ',' << U << ',' << X << "\n";
        }

        curl_global_cleanup();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return "invalid 1";
    }

    return info;
}



string getHTMLsource(string link, int argc, char** argv) { //returns the page you go to if you do ctrl+u on a website (I think this is the background JSON or HTML but idk)
    const char* url = (argc > 1) ? argv[1] : link.c_str();


    std::string body;
    CURL* curl;
    CURLcode res;


    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();


    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "cpp-libcurl/1.0");


        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << "\n";
        }
        // else {
        //     std::cout << body << "\n";  // print page contents
        // }


        curl_easy_cleanup(curl);
    }


    curl_global_cleanup();
    return body;
}

// vector<string> getDeptAbbr(string abbrHTMLsource) {
//     vector<string> abbrs {};
//     return abbrs;
// }


vector<string> getDeptAbbrs(const string& html) { //maybe returning too much rn but I can't find the values that are being returned too much //i figured out they're grad school courses. The "Laws" I just don't feel like dealing with rn if ever. is honestly a useful delimeter if we want to get rid of grad courses bc it's the only instance of "Laws" in entire html source code so could check if abbr is "Laws" and if so break out of loop.
    vector<string> abbrs;
    size_t pos = 0;

    while ((pos = html.find("<a href=", pos)) != string::npos) {
        // Find the closing ">" of the <a ...> tag
        size_t start = html.find(">", pos);
        size_t end   = html.find("-", start);  // dash separates abbr and description

        if (start != string::npos && end != string::npos) {
            string abbr = html.substr(start + 1, end - (start + 1));
            // Trim whitespace
            while (!abbr.empty() && isspace(abbr.back())) abbr.pop_back();
            while (!abbr.empty() && isspace(abbr.front())) abbr.erase(abbr.begin());

            if (abbr.size() == 4 && find(abbrs.begin(), abbrs.end(), abbr) == abbrs.end()) {
                abbrs.push_back(abbr);
            }
        }
        pos = end;
    }

    abbrs.erase(abbrs.begin() + 0); //there was a 2025 at the beginning of the vector this clears

    return abbrs;
}



int main(int argc, char** argv) {

    //getting deptAbbreviations
    // string link = "https://anex.us/grades/?dept=CSCE&number=120"; //for testing 
    string link = "https://catalog.tamu.edu/undergraduate/course-descriptions/#B";

    // cout << getHTMLsource(link, argc, argv) << endl;

    vector<string> deptAbbrs = getDeptAbbrs(getHTMLsource(link, argc, argv));
    
    for (string& deptAbbr : deptAbbrs) { //trying to use reference to save on memory
        // cout << deptAbbr << endl;
        continue; //if only want to 
        for (int num=120; num<121; num++) {
            std::cout << "----------------------------------------------------------\n\n\n\n\n\n----------------------------------------------------------" << std::endl;
            
            getClassData(deptAbbr, num, argc, argv);
        }
    }

    
    link = "https://catalog.tamu.edu/undergraduate/general-information/university-core-curriculum/";


    return 0; //return not actually necessary for function to run
}



/* to run:
g++ fetch.cpp -o fetch -lcurl
./fetch
*/
