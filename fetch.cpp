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



static inline string trim(const string& s) {
    const char* ws = " \t\n\r";
    size_t a = s.find_first_not_of(ws);
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(ws);
    return s.substr(a, b - a + 1);
}

// string buildCategoryLine(const string& html) {
//     // 1) Extract category using the " SCH" marker
//     size_t schPos = html.find(" SCH");
//     if (schPos == string::npos) return ""; // not found

//     size_t catStart = html.rfind('>', schPos);
//     if (catStart == string::npos) return "";

//     string category = trim(html.substr(catStart + 1, schPos - (catStart + 1)));
//     if (category.empty()) return "";

//     // 2) Extract course names between the given delimiters
//     const string leftDelim  = R"(/td><td>)";
//     const string rightDelim = R"(/td><td )";

//     vector<string> courses;
//     size_t pos = 0;
//     while ((pos = html.find(leftDelim, pos)) != string::npos) {
//         size_t start = pos + leftDelim.size();
//         size_t end = html.find(rightDelim, start);
//         if (end == string::npos) break; // no right bound
//         string course = trim(html.substr(start, end - start));
//         if (!course.empty()) courses.push_back(course);
//         pos = end + rightDelim.size();
//     }

//     // 3) Build "Category: a, b, c"
//     string out = category + ": ";
//     for (size_t i = 0; i < courses.size(); ++i) {
//         out += courses[i];
//         if (i + 1 < courses.size()) out += ", ";
//     }
//     return out;
// }

// vector<string> parseCategories(const string& html) {
//     vector<string> results;
//     size_t pos = 0;

//     const string leftDelim  = R"(/td><td>)";
//     const string rightDelim = R"(/td><td )";

//     while (true) {
//         // Find next category marker
//         size_t schPos = html.find(" SCH", pos);
//         if (schPos == string::npos) break;

//         size_t catStart = html.rfind(">", schPos);
//         if (catStart == string::npos) break;
//         string category = trim(html.substr(catStart + 1, schPos - (catStart + 1)));

//         // Gather course names until the next </table>
//         vector<string> courses;
//         size_t tableEnd = html.find("</table>", schPos);
//         size_t coursePos = schPos;

//         while ((coursePos = html.find(leftDelim, coursePos)) != string::npos) {
//             if (tableEnd != string::npos && coursePos > tableEnd) break;
//             size_t start = coursePos + leftDelim.size();
//             size_t end = html.find(rightDelim, start);
//             if (end == string::npos) break;
//             // string course = trim(html.substr(start, end - start));
//             string course = trim(html.substr(start, end - start - 1));
//             if (!course.empty()) courses.push_back(course);
//             coursePos = end + rightDelim.size();
//         }

//         // Build line "Category: course1, course2..."
//         string line = category + ": ";
//         for (size_t i = 0; i < courses.size(); ++i) {
//             line += courses[i];
//             if (i + 1 < courses.size()) line += ", ";
//         }
//         results.push_back(line);

//         pos = schPos + 4; // move past this SCH
//     }

//     return results;
// }

vector<vector<string>> parseCategories(const string& html) { //should eventually move this func and trim() (maybe on trim()) to getCores()
    vector<vector<string>> results;
    size_t pos = 0;

    const string leftDelim  = R"(/td><td>)";
    const string rightDelim = R"(/td><td )";

    while (true) {
        // Find next category marker
        size_t schPos = html.find(" SCH", pos);
        if (schPos == string::npos) break;

        size_t catStart = html.rfind(">", schPos);
        if (catStart == string::npos) break;
        string category = trim(html.substr(catStart + 1, schPos - (catStart + 1)));

        // Gather course names until the next </table>
        vector<string> row;
        row.push_back(category);

        size_t tableEnd = html.find("</table>", schPos);
        size_t coursePos = schPos;

        while ((coursePos = html.find(leftDelim, coursePos)) != string::npos) {
            if (tableEnd != string::npos && coursePos > tableEnd) break;
            size_t start = coursePos + leftDelim.size();
            size_t end = html.find(rightDelim, start);
            if (end == string::npos) break;
            // string course = trim(html.substr(start, end - start));
            string course = trim(html.substr(start, end - start - 1));
            if (!course.empty()) row.push_back(course);
            coursePos = end + rightDelim.size();
        }

        results.push_back(row);
        pos = schPos + 4; // move past this " SCH"
    }

    return results;
}



vector<vector<string>> getCores (const string& html) {
    //get rid of instances of school by checking for "SCH" (all caps) //also found a list of all the depts full names on this page but idk if that's useful
    //R"("hourscol">)" is the unique identifier that preceeds the number of hours for a class
    //R"(/td><td>)" is the unique identifier that appears before each course name
    size_t pos = 0;
    const string headerChecker = R"(onClick="showSection('text', this);"><span>)";

    //bypassing useless " SCH" instances
    while ((html.find(headerChecker, pos)) != string::npos) {
        pos = html.find(headerChecker, pos);
        // cout << html.substr(pos, 100) << endl;
        // cout << pos << endl;
        pos += headerChecker.length();
    }
    
    // return {}; //figured out inf loop is second loop
    
    // cout << "wassup" <<endl;
    // cout << html.find(" SCH", pos) <<endl;

    // if (html.find(" SCH", pos) == string::npos) {
    //     cout << "true" << endl;
    // }

    // cout << html.substr(pos, 100) << "\n\n";
    
    // pos += 100;


    // cout << html.find(" SCH", pos) << endl;

    // const string courseNameStart = R"(/td><td>)";
    // const string courseNameEnd = R"(/td><td )";
    
    // size_t nextSCH, start, end;

    // cout << buildCategoryLine(html) << endl;
    // auto lines = parseCategories(html);
    // for (auto& line : lines) {
    //     cout << line << "\n\n" << endl;
    // }


    vector<vector<string>> results;
    // size_t pos = 0; //can comment out bc pos already where it needs to be

    const string leftDelim  = R"(/td><td>)";
    const string rightDelim = R"(/td><td )";

    while (true) {
        // Find next category marker
        size_t schPos = html.find(" SCH", pos);
        if (schPos == string::npos) break;

        size_t catStart = html.rfind(">", schPos);
        if (catStart == string::npos) break;
        string category = trim(html.substr(catStart + 1, schPos - (catStart + 1)));

        // Gather course names until the next </table>
        vector<string> row;
        row.push_back(category);

        size_t tableEnd = html.find("</table>", schPos);
        size_t coursePos = schPos;

        while ((coursePos = html.find(leftDelim, coursePos)) != string::npos) {
            if (tableEnd != string::npos && coursePos > tableEnd) break;
            size_t start = coursePos + leftDelim.size();
            size_t end = html.find(rightDelim, start);
            if (end == string::npos) break;
            // string course = trim(html.substr(start, end - start));
            string course = trim(html.substr(start, end - start - 1));
            if (!course.empty()) row.push_back(course);
            coursePos = end + rightDelim.size();
        }

        results.push_back(row);
        pos = schPos + 4; // move past this " SCH"
    }

    // return results; //not returning






    // auto coreVecs = parseCategories(html); //ig auto auto-assigns types
    
    // for (const auto& coreVec : coreVecs) {


    //prints out 2D vector

    for (const auto& coreVec : results) {
        cout << coreVec.at(0) << '\n' << endl;
        for (size_t i=1; i<coreVec.size(); i++) {
            cout << coreVec.at(i) << ";; ";
        }
        cout << "\n\n\n" << endl; //flushing text and seperating core credits
    }


    return results;

    // while ((pos = html.find(" SCH", pos)) != string::npos) {
    // // while ((html.find(" SCH", pos)) != string::npos) {

    //     nextSCH = html.find(" SCH", pos+1);
    //     // nextSCH = pos;
    //     cout << "nextSCH1: " << nextSCH << endl;
    //     cout << "start1: " << html.find(courseNameStart, pos) << endl;

    //     if (nextSCH > (start = html.find(courseNameStart, pos))) {
    //         cout << "True" << endl;
    //     }
    //     // return {}; //troublshooting

    //     while (nextSCH > (start = html.find(courseNameStart, pos))) { //sectioning off the vectors
    //         // cout << html.substr(start+courseNameStart.length(), end=html.find(courseNameEnd, pos)) << endl;
    //         cout << "nextSCH: " << nextSCH << endl;
    //         // cout << "start+len: " << start+courseNameStart.length() << endl;
    //         cout << html.substr(start, 100) << "\n\n";
    //         cout << "end: " << (end=html.find(courseNameEnd, pos)) << endl;

            
            
    //         // end = html.find("-", start);
    //         pos = end;
    //     }
        

    // }

    // return {};
} 


int main(int argc, char** argv) {

    //getting deptAbbreviations
    // string link = "https://anex.us/grades/?dept=CSCE&number=120"; //for testing 
    string link = "https://catalog.tamu.edu/undergraduate/course-descriptions/#B";

    // cout << getHTMLsource(link, argc, argv) << endl;

    vector<string> deptAbbrs = getDeptAbbrs(getHTMLsource(link, argc, argv));
    
    for (const string& deptAbbr : deptAbbrs) { //trying to use reference to save on memory
        // cout << deptAbbr << endl;
        continue; //if only want to 
        for (int num=120; num<121; num++) {
            std::cout << "----------------------------------------------------------\n\n\n\n\n\n----------------------------------------------------------" << std::endl;
            
            getClassData(deptAbbr, num, argc, argv);
        }
    }

    
    link = "https://catalog.tamu.edu/undergraduate/general-information/university-core-curriculum/";
    // cout << getHTMLsource(link, argc, argv) << endl;
    getCores(getHTMLsource(link, argc, argv));

    return 0; //return not actually necessary for function to run
}



/* to run:
g++ fetch.cpp -o fetch -lcurl
./fetch
*/
