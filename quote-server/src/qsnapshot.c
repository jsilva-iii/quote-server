//Copyright (C) Apr 2012 Jorge Silva, Gordon Tani
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either version 2
//of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA./*
//
// This is part of a Multicast Quote Feed Server for :
// Toronto TL1 and Montreak HSVF depth quotes
// This will take snapshots of current quotes in memory

#include "qsnapshot.h"
#include "qfh.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

//snapshots:
//{
//		interval = {
//start ="09:29:59.5";
//finish ="09:31:59.5";
//interval = 500;   // interval in milliseconds
//				};
//times = ["09:29:59.5" , "09:31:59.5" , "15:39:30.00" ,"15:40:31.00" ,"15:59:00","16:00:00" ,"16:15:0"];
//snapshotdir = "/home/jsilva/temp";
//	};

void qsnapshot(void * pargs) {
	sqsnapshot * snapshot = (sqsnapshot *) pargs;
	const char *snapshot_dir;
	struct timeval intv_start, intv_finish;
	long intv_interval;
	useconds_t sleep_time = 10000;		// sleep for 10 milliseconds
	int can_write = 0;
	char lu_str[] = { CONFIG_CONSTR_SZ };
	struct stat lastread, curtime, dirdetails;
	const char * int_st, *int_ft;

	config_init(&snapshot->cfg_file);
	if (config_read_file(&snapshot->cfg_file,
			(const char *) snapshot->configfile) != CONFIG_TRUE) {
		printf("Unable to write to  %s, snapshot service not enabled! \n",
				snapshot->configfile);
	} else {
		snprintf(lu_str, CONFIG_CONSTR_SZ, "%s.snapshotdir", snapshot->refstr);
		config_lookup_string(&snapshot->cfg_file, lu_str, &snapshot_dir);
		if (snapshot_dir != NULL && stat(snapshot_dir, &dirdetails) != 0) {
			if (errno == ENOENT) { // try make the dir
				if (mkdir(snapshot_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
						!= 0) {
				} else
					can_write = 1;
			}
		} else
			can_write = 1;
		if (can_write == 0)
			printf("Unable to write to  %s - %s\n", snapshot_dir,
					strerror(errno));
		// read in the different times
		config_setting_t * ss_settings = config_lookup(&snapshot->cfg_file,
				snapshot->refstr);
		if (ss_settings != NULL ) {
			config_setting_t * interval_settings = config_setting_get_member(
					ss_settings, "interval");
			int_st=NULL;
			int_ft=NULL;
			intv_interval=0;
			config_setting_lookup_string(interval_settings, "start", & int_st);
			config_setting_lookup_string(interval_settings, "finish", & int_ft);
			config_setting_lookup_int64(interval_settings, "start",
					(long *)&intv_interval);
			int hh, mm;
			float ssms, fms, fss;
			sscanf(int_st, "%d:%d:%f", &hh, &mm, &ssms);
			fms = modff(ssms, &fss);
			time_t tm ;
			time(&tm );
			struct tm * stime = localtime(&tm);
			stime->tm_hour = hh;
			stime->tm_min = mm;
			stime->tm_sec = (int) fss;
			intv_start.tv_sec = mktime(&stime);
			intv_start.tv_usec = fms * 1e6;
			//finish time
			sscanf(int_ft, "%d:%d:%f", &hh, &mm, &ssms);
			fms = modff(ssms, &fss);
			stime->tm_hour = hh;
			stime->tm_min = mm;
			stime->tm_sec = (int) fss;
			intv_finish.tv_sec = mktime(&stime);
			intv_finish.tv_usec = fms * 1e6;
		}

	}

}

int init_qsnapshot(sqsnapshot * config) {
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	int retval = pthread_create(&config->pthrd, &attr, (void *) qsnapshot,
			(void *) config);
	pthread_attr_destroy(&attr);
	return retval;
}
