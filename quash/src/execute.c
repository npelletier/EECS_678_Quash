/**
 * @file execute.c
 *
 * @brief Implements interface functions between Quash and the environment and
 * functions that interpret an execute commands.
 *
 * @note As you add things to this file you may want to change the method signature
 */

#include "execute.h"

#include <stdio.h>
#include <stdlib.h>
#include "quash.h"
#include <unistd.h>
#include "deque.h"
#include <sys/wait.h>//for waitpid() call

//need these calls to generate our pid queue per instructions in deque.h
//pid_queue will hold the int process ids
//this queue should probably be generated in run_script, passed to
//		run_process(?) and check_jobs_bg_status so they can see all processes
IMPLEMENT_DEQUE_STRUCT (pid_queue, int);
IMPLEMENT_DEQUE (pid_queue,int);

typedef struct job {
  int job_id;
  pid_queue pid_list;
  bool is_background;//we need this to properly free command_string (only in bg)
  char* command_string;//to print the cmd line input when calling jobs
} job;

IMPLEMENT_DEQUE_STRUCT (job_queue, job);
IMPLEMENT_DEQUE (job_queue, job);

int num_jobs = 0;

job_queue big_job_queue;

int p[2][2];

int current_pipe = 0;

void delete_job(job* job)
{
	//we need a helper function to free memory allocated in each job
	destroy_pid_queue(&job->pid_list);
	free(job->command_string);
}
// Remove this and all expansion calls to it
/**
 * @brief Note calls to any function that requires implementation
 */
#define IMPLEMENT_ME()                                                  \
  fprintf(stderr, "IMPLEMENT ME: %s(line %d): %s()\n", __FILE__, __LINE__, __FUNCTION__)

#define BUFSIZE 256
/***************************************************************************
 * Interface Functions
 ***************************************************************************/

// Return a string containing the current working directory.
char* get_current_directory(bool* should_free) {
  // Get the current working directory. This will fix the prompt path.
  char buf[BUFSIZE];
  *should_free = false;
  long size = pathconf(".", _PC_PATH_MAX);//
  return getcwd(buf, size);
}

// Returns the value of an environment variable env_var
const char* lookup_env(const char* env_var) {

	//**************
	//TODO: MAKE SURE THIS WORKS I HAVEN'T BEEN ABLE TO TRY IT
	//**************

	char *buf = NULL;//buffer to store our return value in
	if((buf = getenv(env_var)) != NULL)//this checks to make sure the variable exists
	{
		return buf;
	}

  return "???";
}

// Check the status of background jobs
void check_jobs_bg_status() {
  // TODO: Check on the statuses of all processes belonging to all background
  // jobs. This function should remove jobs from the jobs queue once all
  // processes belonging to a job have completed.
  if(is_empty_job_queue(&big_job_queue))
  {
	  //dont check for jobs if there are no jobs
	  return;
  }
 int jobLen = length_job_queue(&big_job_queue);
 for(int i=0;i<num_jobs;i++)
 {
	int status;
	bool running = false;
	//iterate through jobs queue
	job temp = pop_front_job_queue(&big_job_queue);
	int pidLen = length_pid_queue(&temp.pid_list);
	for(int j = 0; j < pidLen; j++)
	{
		//check each pid in pid_list to see if still running
		//WNOHANG returns immediately so we don't wait for processes to finish here
		//returns -1 if an error occurs, 0 if process still running, so we
		if(waitpid(pop_front_pid_queue(&temp.pid_list),&status, WNOHANG) > 0)
		{

		}else
		{
			//a process is still running
			running = true;
			//we can't break though, we still have to iterate through the queue
			//and put every process back in order
		}
	}
	if(running)
	{
		print_job_bg_complete(temp.job_id, peek_front_pid_queue(&temp.pid_list), temp.command_string);
		delete_job(&temp);
		num_jobs--;//we have one less job in queue
	}else
	{
		//the job still has running processes, so just continue
		push_back_job_queue(&big_job_queue,temp);
	}
	//have to iterate through entire job queue
  }
  fflush(stdout);
}

// Prints the job id number, the process id of the first process belonging to
// the Job, and the command string associated with this job
void print_job(int job_id, pid_t pid, const char* cmd) {
  printf("[%d]\t%8d\t%s\n", job_id, pid, cmd);
  fflush(stdout);
}

// Prints a start up message for background processes
void print_job_bg_start(int job_id, pid_t pid, const char* cmd) {
  printf("Background job started: ");
  print_job(job_id, pid, cmd);
}

// Prints a completion message followed by the print job
void print_job_bg_complete(int job_id, pid_t pid, const char* cmd) {
  printf("Completed: \t");
  print_job(job_id, pid, cmd);
}

/***************************************************************************
 * Functions to process commands
 ***************************************************************************/
// Run a program reachable by the path environment variable, relative path, or
// absolute path
void run_generic(GenericCommand cmd) {
  // Execute a program with a list of arguments. The `args` array is a NULL
  // terminated (last string is always NULL) list of strings. The first element
  // in the array is the executable
  char* exec = cmd.args[0];
  char** args = cmd.args;

  // TODO: Remove warning silencers
  //(void) exec; // Silence unused variable warning
  //(void) args; // Silence unused variable warning

  // TODO: Implement run generic
  //IMPLEMENT_ME();
  execvp(exec, args);

  perror("ERROR: Failed to execute program");
}

// Print strings
void run_echo(EchoCommand cmd) {
  // Print an array of strings. The args array is a NULL terminated (last
  // string is always NULL) list of strings.
  char** str = cmd.args;

  // TODO: Remove warning silencers
  //(void) str; // Silence unused variable warning

  // TODO: Implement echo
  //IMPLEMENT_ME();

  for (char** i =str; *i != NULL; ++i)
  {
    printf("%s ", *i);
  }
  printf("\n");


  // Flush the buffer before returning
  fflush(stdout);
}

// Sets an environment variable
void run_export(ExportCommand cmd) {
  // Write an environment variable
  const char* env_var = cmd.env_var;
  const char* val = cmd.val;

  // TODO: Remove warning silencers
  //(void) env_var; // Silence unused variable warning
  //(void) val;     // Silence unused variable warning

  // TODO: Implement export.
  // HINT: This should be quite simple.
  //IMPLEMENT_ME();
  setenv(env_var, val, 1);
}

// Changes the current working directory
void run_cd(CDCommand cmd) {
  // Get the directory name
  const char* dir = cmd.dir;

  // Check if the directory is valid
  if (dir == NULL) {
    perror("ERROR: Failed to resolve path");
    return;
  }

  if(chdir(dir) == 0)//if we can successfully change working directory
  {
	  //setenv(variable name, new value, overwrite)
	  //sets old working directory to previous directory,
	  char* OLD_PWD;
	  setenv(OLD_PWD,PWD,1);
	  setenv(PWD,dir,1);
  }
  fflush(stdout);
}

// Sends a signal to all processes contained in a job
void run_kill(KillCommand cmd) {
  int signal = cmd.sig;
  int job_id = cmd.job;

  // TODO: Remove warning silencers
  (void) signal; // Silence unused variable warning
  (void) job_id; // Silence unused variable warning

  // TODO: Kill all processes associated with a background job
  IMPLEMENT_ME();
}


// Prints the current working directory to stdout
void run_pwd() {
	char *pwd_str;//char array to hold our directory
    //bzero(pwd_str,BUFSIZE);//zero every entry
    bool should_free;
    pwd_str = get_current_directory(&should_free);
    printf("%s\n",pwd_str);
    if(should_free)
    {
  	  free(pwd_str);
    }
    fflush(stdout);
}

// Prints all background jobs currently in the job list to stdout
void run_jobs() {
  // TODO: Print background jobs
  //IMPLEMENT_ME();
  if (num_jobs == 0)
  {
	  return;
  }
  printf("\n");
  for (int i = 0; i < num_jobs; i++)
  {
	  job temp = pop_front_job_queue(&big_job_queue);
	  printf("%d\t%d\t%s\n",i,peek_front_pid_queue(&temp.pid_list),temp.command_string);
	  push_back_job_queue(&big_job_queue,temp);
  }

  // Flush the buffer before returning
  fflush(stdout);
}

/***************************************************************************
 * Functions for command resolution and process setup
 ***************************************************************************/

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for child processes.
 *
 * This version of the function is tailored to commands that should be run in
 * the child process of a fork.
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void child_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case GENERIC:
    run_generic(cmd.generic);
    break;

  case ECHO:
    run_echo(cmd.echo);
    break;

  case PWD:
    run_pwd();
    break;

  case JOBS:
    run_jobs();
    break;

  case EXPORT:
  case CD:
  case KILL:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for the quash process.
 *
 * This version of the function is tailored to commands that should be run in
 * the parent process (quash).
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void parent_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case EXPORT:
    run_export(cmd.export);
    break;

  case CD:
    run_cd(cmd.cd);
    break;

  case KILL:
    run_kill(cmd.kill);
    break;

  case GENERIC:
  case ECHO:
  case PWD:
  case JOBS:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

/**
 * @brief Creates one new process centered around the @a Command in the @a
 * CommandHolder setting up redirects and pipes where needed
 *
 * @note Processes are not the same as jobs. A single job can have multiple
 * processes running under it. This function creates a process that is part of a
 * larger job.
 *
 * @note Not all commands should be run in the child process. A few need to
 * change the quash process in some way
 *
 * @param holder The CommandHolder to try to run
 *
 * @sa Command CommandHolder
 */
void create_process(CommandHolder holder, pid_queue* pid_list) {
  // Read the flags field from the parser
  bool p_in  = holder.flags & PIPE_IN;
  bool p_out = holder.flags & PIPE_OUT;
  bool r_in  = holder.flags & REDIRECT_IN;
  bool r_out = holder.flags & REDIRECT_OUT;
  bool r_app = holder.flags & REDIRECT_APPEND; // This can only be true if r_out
                                               // is true

  // TODO: Remove warning silencers
  (void) p_in;  // Silence unused variable warning
  (void) p_out; // Silence unused variable warning
  (void) r_in;  // Silence unused variable warning
  (void) r_out; // Silence unused variable warning
  (void) r_app; // Silence unused variable warning

  // TODO: Setup pipes, redirects, and new process
  // IMPLEMENT_ME();

  int pid;
  FILE* file;

  pid = fork();
  if(pid == 0)
  {
    child_run_command(holder.cmd); // This should be done in the child branch of a fork
    if(r_in)
    {
      fopen(holder.redirect_in, "r");
      dup2(fileno(file), STDIN_FILENO);
      close(file);
    }
    if(r_out)
    {
      fopen(holder.redirect_out, "w");
      dup2(fileno(file), STDOUT_FILENO);
      close(file);
    }
    if(r_app)
    {
      fopen(holder.redirect_out, "a");
      dup2(fileno(file), STDOUT_FILENO);
      close(file);
    }
    if(p_in)
    {
      close(p[current_pipe%2][1]);
      dup2(p[current_pipe%2][0], STDIN_FILENO);
    }
    if(p_out)
    {
      dup2(p[(current_pipe+1)%2][1], STDOUT_FILENO);
      close(p[(current_pipe+1)%2][0]);
    }
  }
  else
  {
    push_back_pid_queue(&pid_list,pid);
    parent_run_command(holder.cmd); // This should be done in the parent branch of a fork
  }

  current_pipe++;


  //parent_run_command(holder.cmd); // This should be done in the parent branch of
                                  // a fork
  //child_run_command(holder.cmd); // This should be done in the child branch of a fork
}

// Run a list of commands
void run_script(CommandHolder* holders) {
  if (holders == NULL)
  {
    return;
  }

  if(num_jobs == 0)
  {
	big_job_queue = new_job_queue(1);
  }

  check_jobs_bg_status();

  if (get_command_holder_type(holders[0]) == EXIT &&
      get_command_holder_type(holders[1]) == EOC) {
    end_main_loop();
    return;
  }
  job_queue* jobs_ptr = &big_job_queue;

  job this_job = {.job_id = ++num_jobs,.pid_list = new_pid_queue(1), .command_string = get_command_string()};

  CommandType type;
  // Run all commands in the `holder` array
  for (int i = 0; (type = get_command_holder_type(holders[i])) != EOC; ++i)
    create_process(holders[i],&this_job.pid_list);

  if (!(holders[0].flags & BACKGROUND)) {
    // Not a background Job
    if(!is_empty_pid_queue(&this_job.pid_list))
    {
    	int status;
  	   waitpid(peek_back_pid_queue(&this_job.pid_list),&status,1);
      // TODO: Wait for all processes under the job to complete
      //IMPLEMENT_ME();
    }
	delete_job(&this_job);
  }
  else {
	  push_back_job_queue(&big_job_queue,this_job);
	  num_jobs++;
    // A background job.
    // TODO: Push the new job to the job queue
    //IMPLEMENT_ME();

    // TODO: Once jobs are implemented, uncomment and fill the following line
      print_job_bg_start(this_job.job_id, peek_front_pid_queue(&this_job.pid_list), this_job.command_string);
  }
  if(num_jobs == 0)
  {
	  //this makes it easier (dumber) dealing with job queue deletionc
	  destroy_job_queue(&big_job_queue);
  }
}
