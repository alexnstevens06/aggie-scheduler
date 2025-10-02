#include <vector>
#include <string> 
#include <map>

using std::string, std::vector, std::map;

struct course{
    vector<string> past_professors;//could technically be constructed later from course results, probably best to also have stored here
    vector<string> current_professors;//might get removed if we can't use the howdy portal stuff
    double average;//average including all sections
    double honors_average;//average of only honors sections
    double regular_average;//average of non honors sections

    vector<string> immediate_prerequisites;
    vector<string> concurrent_requirements; //can either have completed in past or have concurrent enrollment
    vector<string> satisfactions;
    
    
    //vector<custom_struct> full_prerequisites;


    vector<result_course> annex_results;
};

//meant to store a single data point from anex.us
struct result_course{
    double average;//average
    string professor;//professor
    bool isHonors;//honors vs non honors
    string dates;//semesert and year
    
    //number of letter grades
    map<char, int> letter_grades;

};
