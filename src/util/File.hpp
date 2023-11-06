#include <filesystem>
#include <fstream>
#include <iostream>

class File{
    public:
        File(std::string name,bool bin = false) : m_name(name),m_bin(bin) {}

        ~File(){
            close();
        }

        bool exists(){
            return std::filesystem::exists(m_name);
        }

        std::string read(){
            open();
            std::stringstream strbuf;
            strbuf << m_stream.rdbuf();
            m_stream.rdbuf();
            return strbuf.str();
        }

        void write(std::string str){
            open();
            m_stream.write(str.c_str(),sizeof(str));
        }

        bool open(){
            if(!m_open){
                if(!std::filesystem::exists(m_name)){
                    std::cerr << "Could not find file: " << m_name << ".\nMaybe use openOrCreate()" << std::endl;
                    exit(EXIT_FAILURE);
                }
                m_stream = new *std::fstream(m_name);
                m_open = true;
                if(!m_stream.is_open()){
                    std::cerr << "Could not open: " << m_name << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            return m_open;
        }

        bool openOrCreate(){
            if(!m_open){
                m_stream = new *std::fstream(m_name);
                m_open = true;
                if(!m_stream.is_open()){
                    std::cerr << "Could not open: " << m_name << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            return m_open;
        }

        void close(){
            m_stream.close();
            m_open = false;
        }

    private:
        std::fstream m_stream;
        std::string m_name;
        bool m_open;
        bool m_bin;
};
