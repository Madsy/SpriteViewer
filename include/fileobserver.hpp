#ifndef FILEOBS_HPP_GUARD
#define FILEOBS_HPP_GUARD

#include <errno.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/inotify.h>
#include <climits>
#include <cstring>
#include <string>
#include <vector>
#include <exception>
#include <stdexcept>

class file_observer
{
public:
	file_observer(const std::string& fileName);
	virtual ~file_observer();
	void poll();
protected:
	virtual void on_write()=0;
private:
	int p_Inotify_Fd;
	int p_File_Fd;
	std::string p_dir_name;
	std::string p_file_name;	
};

#endif

