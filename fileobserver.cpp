#include <libgen.h>
#include <algorithm>
#include "fileobserver.hpp"

file_observer::file_observer(const std::string& fileName)
	: p_Inotify_Fd(-1), p_File_Fd(-1)
{
	//split full filename path into filename and path
	//stupid functions modify the input buffer!
	char tmpPathBuf[PATH_MAX];
	std::fill(tmpPathBuf, tmpPathBuf+PATH_MAX, 0);
	std::copy(fileName.begin(), fileName.end(), tmpPathBuf);
	p_dir_name = std::string(dirname(tmpPathBuf));
	//must reset input string every time
	std::fill(tmpPathBuf, tmpPathBuf+PATH_MAX, 0);
	std::copy(fileName.begin(), fileName.end(), tmpPathBuf);
	p_file_name = std::string(basename(tmpPathBuf));

	p_Inotify_Fd = inotify_init1(IN_NONBLOCK);
	if(p_Inotify_Fd == -1){
		int error = errno;
		std::string error_string(strerror(error));
		std::runtime_error ex(error_string);
		throw ex;
	}
	p_File_Fd = inotify_add_watch(p_Inotify_Fd, p_dir_name.c_str(), IN_CLOSE_WRITE);
	if(p_File_Fd == -1){
		int error = errno;
		std::string error_string(strerror(error));
		std::runtime_error ex(error_string);
		throw ex;
	}
}

file_observer::~file_observer(){
	if(p_File_Fd != -1 && p_Inotify_Fd != -1){
		//ignore return value here
		//we can't use it here anyway
		inotify_rm_watch(p_Inotify_Fd, p_File_Fd);
	}
	if(p_Inotify_Fd != -1){
		close(p_Inotify_Fd);
	}
}

void file_observer::poll(){
	fd_set fds;
	timeval tv;
	inotify_event* fd_event = 0;
	std::vector<unsigned char> fd_event_buffer;
	const size_t fd_event_buffer_size = sizeof(inotify_event) + NAME_MAX + 1;
	const size_t fd_event_size = sizeof(inotify_event);
	int ret;
	int error;

	FD_ZERO(&fds);
	FD_SET(p_Inotify_Fd, &fds);

	tv.tv_sec = 0;
	tv.tv_usec = 2000; //1000 == 1ms timeout

	ret = select(p_Inotify_Fd+1, &fds, NULL, NULL, &tv);

	if (ret == -1){
		error = errno;
		std::string error_string(strerror(error));
		std::runtime_error ex(error_string);
		throw ex;
	} else if(ret && FD_ISSET(p_Inotify_Fd, &fds)){
		fd_event_buffer.resize(fd_event_buffer_size);
		ret = read(p_Inotify_Fd, &fd_event_buffer[0], fd_event_buffer_size);
		if (ret == -1){
			error = errno;
			std::string error_string(strerror(error));
			std::runtime_error ex(error_string);
			throw ex;
		}
		//we're lazy right now
		if(ret < fd_event_size){
			return;
		}

		fd_event = (inotify_event*)&fd_event_buffer[0];
		if(fd_event->len > 0){
			std::string file_name(fd_event->name);
			if((fd_event->mask & IN_CLOSE_WRITE) && (fd_event->wd == p_File_Fd) && (file_name == p_file_name)){
				on_write(); //call the event callback
			}
		}
	}
}



