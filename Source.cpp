#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/range/iterator_range.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>

using namespace boost::filesystem;
using namespace std;

class WrongOutputException : public exception {
public:
	WrongOutputException(const char* msg) : exception(msg) {}
};

class EmptyDirectoryException : public exception {
public:
	EmptyDirectoryException(const char* msg) : exception(msg) {}
};

mutex m_mutex;

class FileReader
{
public:
	FileReader(string directory, string output_filename): directory(directory), output_filename(output_filename) {
		boost::filesystem::directory_iterator begin(directory);
		boost::filesystem::directory_iterator end;
		for (; begin != end; ++begin) {
			boost::filesystem::file_status fs =
				boost::filesystem::status(*begin); 
			stringstream ss;
			ss << *begin;
			string s = ss.str();	// "directory" (shouldn't be "")
			s.erase(s.begin());		// delete first "
			s.erase(s.end() - 1);	// delete last "
			files.push_back(s);			
		}
	}

	// copy constructor
	FileReader(FileReader& f) {
		this->directory = f.getDirectory();
		this->files = f.getFiles();
	}

	// move constructor
	FileReader(FileReader&& f) {
		this->directory = f.getDirectory();
		this->files = f.getFiles();
	}

	void parseFilesWithoutMultithreading() {
		if (files.empty()) throw exception("Empty directory!");
		for (auto f : files) {
			ifstream file;
			file.open(f);
			string text;
			while (getline(file, text)) {	// If you want read line in file, 
				vector<string> v_text;		// you can don't use split and push text in output.
				boost::split(v_text, text, boost::is_any_of(" "));
				ofstream output;
				output.open(output_filename, std::ios::app);
				for (auto v : v_text) {
					output << v << " ";
				}
				output << "\n";
				output.close();
			}
			file.close();
		}
	}

	void parseFiles() {
		if (files.empty()) throw exception("Empty directory!");
		vector<thread> threads(files.size());
		for (int i = 0; i < files.size(); ++i) {
			threads.at(i) = thread([&]() {
				for (int i = 0; i < files.size(); ++i) {
					ifstream file;
					file.open(files[i]);
					string text;
					while (getline(file, text)) {	// If you want read line in file, 
						vector<string> v_text;		// you can don't use split and push text in output.
						boost::split(v_text, text, boost::is_any_of(" "));
						ofstream output;
						output.open(output_filename, std::ios::app);
						m_mutex.lock();
						for (auto v : v_text) {
							output << v << " ";
						}
						output << "\n";
						m_mutex.unlock();
						output.close();
					}
					file.close();
				}
				});
		}
		for (int i = 0; i < files.size(); ++i) {
			threads.at(i).join();
		}
	}

	vector<string> getFiles() {
		return files;
	}

	string getDirectory() {
		return directory;
	}

	void printFiles() {
		if (files.empty()) throw exception("Empty directory!");
		cout << "Files in directory:\n";
		for (auto f : files)
			cout << f << endl;
	}

	void testParsingMethods(FileReader m_reader) {
		auto begin = chrono::steady_clock::now();
		m_reader.parseFiles();
		auto end = chrono::steady_clock::now();
		cout << "Parse File =                 " << end - begin << endl;
		begin = chrono::steady_clock::now();
		m_reader.parseFilesWithoutMultithreading();
		end = chrono::steady_clock::now();
		cout << "Parse File without Threads = " << end - begin << endl;
	}

	~FileReader() {}

private:
	string directory;
	string output_filename;
	vector<string> files;

	// In the future don't use lambda expression in 88-108 lines and have this parser method
	void parser(string& filename) {
		for (int i = 0; i < files.size(); ++i) {
			ifstream file;
			file.open(files[i]);
			string text;
			while (getline(file, text)) {	// If you want read line in file, 
				vector<string> v_text;		// you can don't use split and push text in output.
				boost::split(v_text, text, boost::is_any_of(" "));
				ofstream output;
				output.open(output_filename, std::ios::app);
				m_mutex.lock();
				for (auto v : v_text) {
					output << v << " ";
				}
				output << "\n";
				m_mutex.unlock();
				output.close();
			}
			file.close();
		}
	}

};

int main() {
	setlocale(0, "rus");
	string directory;
	string output_file;
	cout << "Enter the directory and output file: ";
	try {
		cin >> directory >> output_file;
		regex reg("[\\d\\w]+.txt");
		if (regex_match(output_file, reg)) {
			FileReader m_reader(directory, output_file);
			m_reader.parseFiles();
		}
		else throw exception("Wrong output file name!");
	}
	catch (WrongOutputException& ex) {
		cout << ex.what() << endl;
	}
	catch(exception& ex) {
		cerr << ex.what() << endl;
	}}