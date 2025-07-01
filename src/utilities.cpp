
#include "utilities.h"
#include "classes.h"

// namespace fs = std::experimental::filesystem;

namespace utils
{
    void splash_screen()
    {
        std::cout << "" << std::endl;
        std::cout << "Welcome to SolventMixtures!" << std::endl;
        std::cout << "" << std::endl;
    }

    void silent_shell(const char* cmd)
    {
        std::array<char, 128> buffer;
        std::string result;
        std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
        if (!pipe) throw std::runtime_error("popen() failed!");
        while (!feof(pipe.get())) {
            if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
                result += buffer.data();
        }
    }

    std::string GetSysResponse(const char* cmd)
    {
        std::array<char, 128> buffer;
        std::string result;
        std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
        if (!pipe) throw std::runtime_error("popen() failed!");
        while (!feof(pipe.get())) {
            if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
                result += buffer.data();
        }
        return result;
    }

    bool CheckProgAvailable(const char* program)
    {
            std::string result;
            std::string cmd;
            cmd = "which ";
            cmd += program;
            result=GetSysResponse(cmd.c_str());
            if (result.empty())
            {
                std::cout << "Missing program: " << program << std::endl;
                return false;
            }
            return true;
    }

    void write_to_file(std::string inputfilename, std::string buffer)
    {
        std::ofstream outFile;
        outFile.open(inputfilename,std::ios::out);
        outFile << buffer;
        outFile.close();
    }

    void append_to_file(std::string inputfilename, std::string buffer)
    {
        if (!CheckFileExists(inputfilename))
        {
            write_to_file(inputfilename,"");
        }
        std::ofstream outFile;
        outFile.open(inputfilename,std::ios::app);
        outFile << buffer;
        outFile.close();
    }

    bool IsFlag(char* bigstring) //checks if a string is a command line flag or a value.
    {
        if (bigstring[0] == '-')
            return true;
        return false;
    }

    void ReadArgs(int argc, char** argv, std::vector<std::vector<std::string>>& flags)
    {
        int j = 0;
        for (int i = 1; i < argc; i++)
        {
            if (IsFlag(argv[i]))
            {
                j++;
                flags.push_back({argv[i]});
            }
            else
                flags[j].push_back(argv[i]);
        }
    }

    int FindFlag(std::vector<std::vector<std::string>>& flags, char* target)
    {
        for (int i = 1; i<flags.size(); i++)
        {
            if (flags[i][0] == target)
                return i;
        }
        return 0;
    }
 
    bool CheckFileExists(std::string filename)
    {
        if ( fs::exists(filename) )
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
 
    std::string GetTimeAndDate()
    {
        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::stringstream buffer;
        buffer.str("");
        buffer << std::put_time(&tm, "%Y.%m.%d %H:%M:%S");
        return buffer.str();
    }
 
    int is_empty(const char *s) 
    {
        while (*s != '\0') 
        {
            if (!isspace((unsigned char)*s))
            return 0;
            s++;
        }
        return 1;
    }
 
    std::string LastLineOfFile(std::string filename)
    {
        std::ifstream fin;
        fin.open(filename);
        std::string line;
        std::vector<std::string> lines_in_order;
        while (std::getline(fin, line))
        {   
            if ( !is_empty(line.c_str()) )
            {
                lines_in_order.push_back(line);
            }
        }
        fin.close();
        return lines_in_order[lines_in_order.size()-1];
    }

    void mdout_to_csv(std::string filename,std::string csv_file)
    {
        std::stringstream buffer;
        buffer.str("");
        std::ifstream mdout(filename);
        std::string line;
        std::stringstream dummy;
        float time_adjust = 0.0;
    
        // if coldequil.csv doesn't exist, make it.  otherwise, obtain the first value on the last line for time_adjust.
        if (!utils::CheckFileExists(csv_file))
        {
            buffer.str("");
            buffer << "TIME(PS),TEMP(K),PRESS,Etot,EKtot,EPtot,BOND,ANGLE,DIHED,1-4 NB,1-4 EEL,VDWAALS,EELEC,EHBOND,RESTRAINT,EKCMT,VIRIAL,VOLUME,Density";
            utils::write_to_file(csv_file,buffer.str());
            buffer.str("");
            buffer << std::endl;
        }
        else
        {
            std::string lastline = utils::LastLineOfFile(csv_file);
            time_adjust = stof(lastline.substr(0,lastline.find_first_of(","))) + 0.001;
            std::cout << "DEBUG: Time Adjust Value found: " << time_adjust << std::endl;
        }

        // Get to the results section, skip 3 lines, break out.
        while (std::getline(mdout,line))
        {
            if (line.find("4.  RESULTS") != std::string::npos)
            {
                std::getline(mdout,line);
                std::getline(mdout,line);
                std::getline(mdout,line);
                break;
            }
        }

        while (std::getline(mdout,line))
        {
            std::string key;
            std::string val;
            if (line.find("A V E R A G E S   O V E R") != std::string::npos) // end of results, exit file.
            {
                utils::append_to_file(csv_file, buffer.str());
                buffer.str("");
                break;
            }
            if (line.find("----") != std::string::npos) //end of current batch, time for a new line
            {
                std::getline(mdout,line);
                std::getline(mdout,line);
                utils::append_to_file(csv_file, buffer.str());
                buffer.str("");
                buffer << std::endl;
            }
            std::replace(line.begin(), line.end(), '=', ' ');
            if (line.find("NMR restraints") != std::string::npos) //skip NMR restraints lines
            {
                continue;
            }
            if (line.find("EAMBER") != std::string::npos) //skip EAMBER lines
            {
                continue;
            }
            if (line.find_first_not_of("\t\n ") == std::string::npos) //empty string or whitespace string
            {
                continue;
            }
            if (line.find("NSTEP") != std::string::npos)
            {
                //NSTEP =        0   TIME(PS) =       0.000  TEMP(K) =    14.66  PRESS =   -89.7
                int linelen = line.length();
                int marker1 = line.find("TIME(PS)");
                int marker2 = line.find("TEMP(K)");
                int marker3 = line.find("PRESS");
                std::string val1 = line.substr(marker1 + 8, marker2 - marker1 - 8);
                std::string val2 = line.substr(marker2 + 7, marker3 - marker2 - 7);
                std::string val3 = line.substr(marker3 + 5, linelen - marker3 - 5);
                val1 = val1.substr(val1.find_first_not_of(" "),val1.length());
                val1 = val1.substr(0,val1.find_first_of(" "));
                val2 = val2.substr(val2.find_first_not_of(" "),val2.length());
                val2 = val2.substr(0,val2.find_first_of(" "));
                val3 = val3.substr(val3.find_first_not_of(" "),val3.length());
                val3 = val3.substr(0,val3.find_first_of(" "));
                if (stod(val1) > time_adjust)
                {
                    time_adjust = 0.0;
                }
                buffer << stod(val1) + time_adjust << ", "; // TIME(PS)
                buffer << val2 << ", "; // TEMP(K)
                buffer << val3 << ", "; // PRESS
                utils::append_to_file(csv_file, buffer.str());
                buffer.str("");
            }
            if (line.find("Etot") != std::string::npos)
            {
                //Etot   =   -314475.2949  EKtot   =      2462.7952  EPtot      =   -316938.0901
                int linelen = line.length();
                int marker1 = line.find("Etot");
                int marker2 = line.find("EKtot");
                int marker3 = line.find("EPtot");
                std::string val1 = line.substr(marker1 + 4, marker2 - marker1 - 4);
                std::string val2 = line.substr(marker2 + 5, marker3 - marker2 - 5);
                std::string val3 = line.substr(marker3 + 5, linelen - marker3 - 5);
                val1 = val1.substr(val1.find_first_not_of(" "),val1.length());
                val1 = val1.substr(0,val1.find_first_of(" "));
                val2 = val2.substr(val2.find_first_not_of(" "),val2.length());
                val2 = val2.substr(0,val2.find_first_of(" "));
                val3 = val3.substr(val3.find_first_not_of(" "),val3.length());
                val3 = val3.substr(0,val3.find_first_of(" "));
                buffer << val1 << ", "; // Etot
                buffer << val2 << ", "; // EKtot
                buffer << val3 << ", "; // EPtot
                utils::append_to_file(csv_file, buffer.str());
                buffer.str("");
            }
            if (line.find("DIHED") != std::string::npos)
            {
                //BOND   =       194.9983  ANGLE   =       892.4813  DIHED      =      4381.4538
                int linelen = line.length();
                int marker1 = line.find("BOND");
                int marker2 = line.find("ANGLE");
                int marker3 = line.find("DIHED");
                std::string val1 = line.substr(marker1 + 4, marker2 - marker1 - 4);
                std::string val2 = line.substr(marker2 + 5, marker3 - marker2 - 5);
                std::string val3 = line.substr(marker3 + 5, linelen - marker3 - 5);
                val1 = val1.substr(val1.find_first_not_of(" "),val1.length());
                val1 = val1.substr(0,val1.find_first_of(" "));
                val2 = val2.substr(val2.find_first_not_of(" "),val2.length());
                val2 = val2.substr(0,val2.find_first_of(" "));
                val3 = val3.substr(val3.find_first_not_of(" "),val3.length());
                val3 = val3.substr(0,val3.find_first_of(" "));
                buffer << val1 << ", "; // BOND
                buffer << val2 << ", "; // ANGLE
                buffer << val3 << ", "; // DIHED
                utils::append_to_file(csv_file, buffer.str());
                buffer.str("");
            }
            if (line.find("1-4 NB") != std::string::npos)
            {
                //1-4 NB =      1941.0169  1-4 EEL =     18036.9140  VDWAALS    =     55600.7999
                int linelen = line.length();
                int marker1 = line.find("1-4 NB");
                int marker2 = line.find("1-4 EEL");
                int marker3 = line.find("VDWAALS");
                std::string val1 = line.substr(marker1 + 6, marker2 - marker1 - 6);
                std::string val2 = line.substr(marker2 + 7, marker3 - marker2 - 7);
                std::string val3 = line.substr(marker3 + 7, linelen - marker3 - 7);
                val1 = val1.substr(val1.find_first_not_of(" "),val1.length());
                val1 = val1.substr(0,val1.find_first_of(" "));
                val2 = val2.substr(val2.find_first_not_of(" "),val2.length());
                val2 = val2.substr(0,val2.find_first_of(" "));
                val3 = val3.substr(val3.find_first_not_of(" "),val3.length());
                val3 = val3.substr(0,val3.find_first_of(" "));
                buffer << val1 << ", "; // 1-4 NB
                buffer << val2 << ", "; // 1-4 EEL
                buffer << val3 << ", "; // VDWAALS
                utils::append_to_file(csv_file, buffer.str());
                buffer.str("");
            }
            if (line.find("EELEC") != std::string::npos)
            {
                //EELEC  =    -83661.5787  EHBOND  =         0.0000  RESTRAINT  =         0.0000
                int linelen = line.length();
                int marker1 = line.find("EELEC");
                int marker2 = line.find("EHBOND");
                int marker3 = line.find("RESTRAINT");
                std::string val1 = line.substr(marker1 + 7, marker2 - marker1 - 7);
                std::string val2 = line.substr(marker2 + 6, marker3 - marker2 - 6);
                std::string val3 = line.substr(marker3 + 9, linelen - marker3 - 9);
                val1 = val1.substr(val1.find_first_not_of(" "),val1.length());
                val1 = val1.substr(0,val1.find_first_of(" "));
                val2 = val2.substr(val2.find_first_not_of(" "),val2.length());
                val2 = val2.substr(0,val2.find_first_of(" "));
                val3 = val3.substr(val3.find_first_not_of(" "),val3.length());
                val3 = val3.substr(0,val3.find_first_of(" "));
                buffer << val1 << ", "; // EELEC
                buffer << val2 << ", "; // EHBOND
                buffer << val3 << ", "; // RESTRAINT
                utils::append_to_file(csv_file, buffer.str());
                buffer.str("");
            }
            if (line.find("EKCMT") != std::string::npos)
            {
                //EKCMT  =       748.8663  VIRIAL  =      2298.8197  VOLUME     =    800440.3366
                int linelen = line.length();
                int marker1 = line.find("EKCMT");
                int marker2 = line.find("VIRIAL");
                int marker3 = line.find("VOLUME");
                std::string val1 = line.substr(marker1 + 5, marker2 - marker1 - 5);
                std::string val2 = line.substr(marker2 + 6, marker3 - marker2 - 6);
                std::string val3 = line.substr(marker3 + 6, linelen - marker3 - 6);
                val1 = val1.substr(val1.find_first_not_of(" "),val1.length());
                val1 = val1.substr(0,val1.find_first_of(" "));
                val2 = val2.substr(val2.find_first_not_of(" "),val2.length());
                val2 = val2.substr(0,val2.find_first_of(" "));
                val3 = val3.substr(val3.find_first_not_of(" "),val3.length());
                val3 = val3.substr(0,val3.find_first_of(" "));
                buffer << val1 << ", "; // EKCMT
                buffer << val2 << ", "; // VIRIAL
                buffer << val3 << ", "; // VOLUME
                utils::append_to_file(csv_file, buffer.str());
                buffer.str("");
            }
            if (line.find("Density") != std::string::npos)
            {
                //Density    =         1.0471
                int linelen = line.length();
                int marker1 = line.find("Density");
                std::string val1 = line.substr(marker1 + 7, linelen - marker1 - 7);
                val1 = val1.substr(val1.find_first_not_of(" "),val1.length());
                val1 = val1.substr(0,val1.find_first_of(" "));
                buffer << val1; // Density
                utils::append_to_file(csv_file, buffer.str());
                buffer.str("");
            }
        }
        mdout.close();
    }

    int count_lines_in_file(std::string filename)
    {
        std::string dummy;
        int count = 0;
        std::ifstream file(filename);
        while (getline(file,dummy))
        {
            count ++;
        }
        return count;
    }

    std::string string_between(std::string incoming, std::string first_delim, std::string second_delim)
    {
        unsigned first = incoming.find(first_delim);
        if (first == std::string::npos)
        {
            return incoming;
        }
        unsigned last = incoming.find(second_delim);
        if (last == std::string::npos)
        {
            return incoming.substr(first + 1, incoming.size());
        }
        return incoming.substr(first + 1, last - first - 1);
    }
    
    void compress_and_delete(std::string directory)
    {
        std::stringstream buffer;
        buffer.str("");
        buffer << "tar -czvf " << directory << ".tar.gz "<< directory << "/ && rm -r " << directory << "/";
        silent_shell(buffer.str().c_str());
    }

    std::vector<std::string> sort_files_by_timestamp(std::string directory,std::string pattern)
    {
        std::set <fs::path> sort_by_name;
        for (fs::path p : fs::directory_iterator(directory))
        {
            if (p.extension() == pattern) 
            {
                sort_by_name.insert(p);
            }    
        }
        std::vector<std::string> file_list={};

        for (auto p : sort_by_name)
        {
            std::cout << p << std::endl;
            file_list.push_back(p);
        }
        
        return file_list;
    }
}
