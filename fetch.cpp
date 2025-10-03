#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>

using json = nlohmann::json;
using std::string, std::vector;



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
string getClassData (string deptAbbr, int num, int argc, char** argv) {
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



int main(int argc, char** argv) {

    vector<string> deptAbbreviations = {"AALO","ACCT","AEGD","AFST","AGCJ","AGEC","AGLS","AGSC","AGSM","ALEC","ALED","ANLY","ANSC","ANTH","ARAB","ARCH","AREN","ARSC","ARTS","ASCC","ASIA","ASTR","ATMO","ATTR","BAEN","BESC","BIMS","BIOL","BIOT","BMEN","BUAD","BUSH","BUSN","CARC","CHEM","CHEN","CHIN","CLAS","CLAT","CLEN","COMM","COSC","CPSY","CSCE","CULN","CVEN","DAEN","DCED","DDHS","DHUM","DPHS","ECCB","ECEN","ECMT","ECON","EDAD","EDCI","EDHP","EEBL","EHRD","ENDO","ENDS","ENGL","ENGR","ENGY","ENSS","ENTC","ENTO","ENTR","EPSY","ESET","EVEN","FILM","FINC","FINP","FIVS","FORS","FREN","FSTC","FYEX","GENE","GEOG","GEOL","GEOP","GEOS","GERM","GLST","HBEH","HCPI","HISP","HIST","HLTH","HMGT","HONR","HORT","IBST","IBUS","IDIS","INST","INTA","ISEN","ISTM","ITAL","ITDE","ITSV","JAPN","JOUR","KINE","LAND","LAW","LDEV","LDTC","MASC","MATH","MEEN","MEFB","MEMA","MEPS","MGPT","MKTG","MLST","MMET","MODL","MPHY","MSCI","MSEN","MSTC","MTDE","MUSC","MUST","MXET","NEXT","NRSC","NSEB","NUEN","NURS","NUTR","NVSC","OBIO","OCEN","OCNG","OMFP","OMFR","OMFS","ORTH","PBSI","PEDD","PERF","PERI","PETE","PHAR","PHEB","PHEO","PHIL","PHLT","PHPM","PHSC","PHYS","PLAN","PLPA","POLS","POSC","PROS","PSAA","PVFA","RDNG","RELS","RPTS","RUSS","RWFM","SABR","SCMT","SCSC","SEFB","SENG","SOCI","SOMS","SOPH","SPAN","SPED","SPMT","SPSY","SSEN","STAT","SYEX","TCMG","TEED","TEFB","THEA","UGST","URPN","URSC","VIBS","VIST","VIZA","VLCS","VMID","VPAT","VSCS","VTMI","VTPB","VTPP","WGST","WHMS"};
    
    // vector<string> deptAbbreviations = {"CSCE"};
    // string course[2] = {"CSCE", string(120)};
    // string course[2] = {"CSCE", "120"};
    // string course[2] = {"PHYS", "206"};

    // string course[2];

    for (int deptInd=0; deptInd<deptAbbreviations.size(); deptInd++) {
    // for (int deptInd=0; deptInd<1; deptInd++) {
        // for (int num=100; num<1000; num++) {
        for (int num=100; num<101; num++) {
            std::cout << "----------------------------------------------------------\n\n\n\n\n\n----------------------------------------------------------" << std::endl;
            std::cout << deptAbbreviations.at(deptInd) << num << std::endl;


            // string course[2] = {"CSCE", "120"};
            // course.at(0) = deptAbbreviations.at(deptInd);
            // course[0] = deptAbbreviations.at(deptInd);
            // course[1] = std::to_string(num);
            
            // string course[2] = {deptAbbreviations.at(deptInd), to_string(120)};
            // getClassData(, argc, argv);
            // getClassData(course, argc, argv);
            getClassData(deptAbbreviations.at(deptInd), num, argc, argv);
        }
    }
    

    // getClassData(course, argc, argv);

    // try {
    //     std::string dept   = (argc > 1) ? argv[1] : "CSCE";
    //     std::string number = (argc > 2) ? argv[2] : "120";

    //     curl_global_init(CURL_GLOBAL_DEFAULT);

    //     const std::string url = "https://anex.us/grades/getData/";
    //     std::string fields = "dept=" + dept + "&number=" + number;

    //     std::string response = http_post(url, fields);
    //     json j = json::parse(response);

    //     if (!j.contains("classes") || !j["classes"].is_array()) {
    //         std::cerr << "Unexpected JSON shape, first 2KB:\n"
    //                   << response.substr(0, 2000) << "\n";
    //         curl_global_cleanup();
    //         return 2;
    //     }

    //     const json& rows = j["classes"];

    //     std::cout << "Year,Semester,Prof,Section,GPA,A,B,C,D,F,I,Q,S,U,X\n";
    //     for (const auto& r : rows) {
    //         int    year     = to_int(r, "year");
    //         std::string sem = r.value("semester", "");
    //         std::string prof= r.value("prof", "");
    //         int    section  = to_int(r, "section");
    //         double gpa      = to_double(r, "gpa");

    //         int A = to_int(r,"A"), B = to_int(r,"B"), C = to_int(r,"C"),
    //             D = to_int(r,"D"), F = to_int(r,"F"), I = to_int(r,"I"),
    //             Q = to_int(r,"Q"), S = to_int(r,"S"), U = to_int(r,"U"),
    //             X = to_int(r,"X");

    //         std::cout << year << ',' << sem << ',' << prof << ','
    //                   << section << ',' << gpa << ','
    //                   << A << ',' << B << ',' << C << ','
    //                   << D << ',' << F << ',' << I << ','
    //                   << Q << ',' << S << ',' << U << ',' << X << "\n";
    //     }

    //     curl_global_cleanup();
    //     return 0;
    // } catch (const std::exception& e) {
    //     std::cerr << "Error: " << e.what() << "\n";
    //     return 1;
    // }



    return 0; //return not actually necessary for function to run
}