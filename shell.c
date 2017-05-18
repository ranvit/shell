/**
 * Machine Problem: Shell
 * CS 241 - Fall 2016
 */
/*
#include "format.h"
#include "log.h"
#include "shell.h"

int shell(int argc, char *argv[]) {
  // TODO: This is the entry point for your shell.
  return 0;
}
*/

/**
 * Machine Problem: Shell
 * CS 241 - Spring 2016
 */
#include "format.h"
#include "log.h"
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int built_in(const char * input, Log * daddy);

int built_or_not(const char * input, Log * daddy);

int non_built_in(const char * input, Log * daddy);

void exit_stuff(const char * input, Log * daddy, char * current_working_directory);

int shell(int argc, char *argv[]) {
  // TODO: This is the entry point for your shell.

	extern char * optarg;
	extern int optind;
	int c, err = 0;
	int fflag=0, hflag=0;
	char * hname, *fname;

	Log * cmd_log;
	Log * daddy;

	while ((c = getopt(argc, argv, "f:h:")) != -1){
		switch (c) {

			case 'h':
				hflag = 1;
				hname = optarg;
				break;

			case 'f':
				fflag = 1;
				fname = optarg;
				break;

			case '?':
				err = 1;
				break;
		}
	}
/*
	if(hflag && fflag){
		Log * hist_log = Log_create_from_file(hname);
		Log * cmd_log = Log_create_from_file(fname);
	}
*/
	if(err){
		print_usage();
//		return 0;
		exit(0);
	}

	if(fflag) cmd_log = Log_create_from_file(fname);

	if(hflag) daddy = Log_create_from_file(hname);
	else daddy = Log_create();

//	char * current_working_directory = malloc(64 * sizeof(char));

//	printf("%s\n", hname);

	char * current_working_directory;
	char * input = malloc(sizeof(char) * 8);

	print_shell_owner("bommine2");

	if(fflag){
		int length = (int) Log_size(cmd_log);
		for(int i = 0; i < length; i++){

			signal(SIGINT, SIG_IGN);

			pid_t main_pid = getpid();
			current_working_directory = get_current_dir_name();
			print_prompt(current_working_directory, main_pid);

			printf("%s\n", Log_get_command(cmd_log, i));

			int check = built_or_not(Log_get_command(cmd_log, i), daddy);
			if(check == 1) break;
		}
		if(hflag) Log_save(daddy, hname);
		exit_stuff(input, daddy, current_working_directory);
		Log_destroy(cmd_log);
//		return 0;
		exit(0);
	}

	while(1){

		signal(SIGINT, SIG_IGN);

		pid_t main_pid = getpid();
		current_working_directory = get_current_dir_name();
		print_prompt(current_working_directory, main_pid);

		size_t boop = 8;
		getline(&input, &boop, stdin);
		input[strlen(input) - 1] = '\0';

		int check = built_or_not(input, daddy);
		if(check == 1){
			if(hflag) Log_save(daddy, hname);
			exit_stuff(input, daddy, current_working_directory);
//			return 0;
			exit(0);
		}
//		int big = built_or_not(input);
//		if(big == 1) built_in(input);
//		else non_built_in(input);

	}

}

int built_or_not(const char * input, Log * daddy){
	if( (strncmp(input, "cd ", 3) == 0) || (strncmp(input, "exit", 4) == 0) || (strncmp(input, "!history", 8) == 0) || (strncmp(input, "#", 1) == 0) || (strncmp(input, "!", 1) == 0) ) return built_in(input, daddy);

	else return non_built_in(input, daddy);
}

int built_in(const char * input, Log * daddy){

	if(strncmp(input, "cd ", 3) == 0){
		char * dest = (char *)input+3;
		int check = chdir(dest);
		if(check == -1) print_no_directory(dest);
		Log_add_command(daddy, input);
	}

	else if(strncmp(input, "exit", 4) == 0){
//		free(input);
//		free(current_working_directory);
//		Log_destroy(daddy);
		return 1;
	}

	else if(strncmp(input, "!history", 8) == 0){
		int lines = (int) Log_size(daddy);
		for(int i = 0; i < lines; i++){
			printf("%d	%s\n", i, Log_get_command(daddy, i));
		}
//		Log_add_command(daddy, input);
	}

	else if(strncmp(input, "#", 1) == 0){
		int line = atoi((char *)input+1);

		if(line < 0 || line >= (int)Log_size(daddy)) print_invalid_index();

		else if(line == 0 && *((char *)input+1) != '0') print_invalid_index();	// not necessary

		else{
			const char * old = Log_get_command(daddy, line);

//			Log_add_command(daddy, old);
			printf("%s\n", old);

			int check = built_or_not(old, daddy);
			return check;
		}
	}

	else if(strncmp(input, "!", 1) == 0){
		char * prefix = malloc(strlen(input) + 1);
		strcpy(prefix, (char *)input+1);
		int len = (int) strlen(prefix);

		for(int i = (int) (Log_size(daddy) - 1); i >= 0; i--){
			if(strncmp(prefix, Log_get_command(daddy, i), len) == 0){
//				Log_add_command(daddy, Log_get_command(daddy, i));
				printf("%s\n", Log_get_command(daddy, i));
				int check = built_or_not(Log_get_command(daddy, i), daddy);
				return check;
			}
		}
		print_no_history_match();
	}

	return 0;
}

int non_built_in(const char * input, Log * daddy){

	Log_add_command(daddy, input);	// before fork or after

	int len = strlen(input);
	char amp = input[len-1];

	pid_t demon_child = fork();
	if(demon_child == -1) print_fork_failed();
	if(demon_child > 0){
		print_command_executed(demon_child);
		int status;
		if(amp != '&') waitpid(demon_child, &status, 0);	// else dont wait, run in background
	}

	else{

		signal(SIGINT, SIG_IGN);

		const char * delim = " ";
		size_t numtokens = 0;

		char ** tokens = strsplit(input, delim, &numtokens);

		int exec_check = execvp(tokens[0], tokens);

		if(exec_check == -1){ print_exec_failed(tokens[0]); exit(0);}
	}

	return 0;

}

void exit_stuff(const char * input, Log * daddy, char * current_working_directory){
	int status;
	while (waitpid((pid_t) (-1), &status, WNOHANG) > 0) {}

	free((void *)input);
	free(current_working_directory);
	Log_destroy(daddy);
}
