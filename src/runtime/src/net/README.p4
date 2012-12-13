p4_initenv basically does the following things:
1) Identify if I am a slave node or master node. This can be done
because for all P4 started processes, they all contain an undocumented
flag called "-p4amslave" (besides two other additional options that
tell the slave where is the master node and on which port it is listening
to).

2) We can retrieve such information by checking (p4_local->my_id == 0).
A new function to be added to P4 to check whether the process is the
command line startup process.

3) Strip off other headers such as process group definition file or
construct the default one if there is no such option specified in 
the command line. However, it turns out that the slave processes
do not receive the command line args passed to the master process
at all, nor do they strip off the 3 command line args added by the
master. 

MPICH that built upon P4 solved this problem by broadcasting the
arguments after P4_initenv. (three bcasts: 1. send the argc,
2. send an array of the length of each argv, 3. send a concatenated
string of all the argvs, the receiver is able to safely break down
the argvs from the concatenated string with the previous two pieces 
of information.)

Planed Fix: I assume SPMD style of programming with resource allocation
daemon. That means, executable are located on the same directory
on all machines, all of them get the same set of command line
arguments. No procgroup file will be needed evenutally (in case for
easy testing and experiment, I will use the same format as MPICH).

Current Status:
	Have renamed strip_out_args to p4_strip_out_args and made it public.
	Added two global pointers to the argc and argv provided by main.
	Sending all command arguments to the remote as ssh arguments. (Not
	100% safe.)
	Strip off the first four arguments from slave side.
	Looks pretty okay for simple tests. :)

4) Assigning my_id.

p4_createprocgrp does nothing for slaves. But for the startup master,
it is the place where new processes get spawned onto other machines.

On each machine, P4 will create a listener process and all other processes
will communicate with it through pipe I think (it writes to the process
through ioctl). The cluster master on each machine forks other processes.

I am thinking of putting this listener as a thread instead of a process.
For one reason, it can save some copying from one process to the other.

I am not sure if there could be other benefits besides that.

However, I am short of time, taking P4 doesn't seem
to be that bad considering that MPICH also use the same stuff.

March 30th, it turns out that MPICH has significantly modified
p4_utils.c

P4 basically gives me two ways of buffering a received message:
1) pass in a buffer pointer that is previously allocated by 
p4_msg_alloc().
2) or NULL (That is what I am going to use)

In either cases, the messages have to be buffered into the P4 system
memory. The difference is that 2) simply returns back the P4 system
buffer and 1) simply copy the data from the P4 system buffer to the
user buffer (and then possibly reuse the system buffer).

In the case of sending a message, again internal copying is
inevitable if we have no control over how the user data buffer is
allocated. (I am using p4_send(). )

Added p4_reduce in p4_broadcast.c, need to test it. For each different
root, we construct a communication tree. 

Changed slave's full path to 1024 bytes long, this is important because
the old 100 bytes is definitely too short when we have a deep directory
hierarchy and long executable name.

Also changed the user name to be 128 bytes long instead of the old 10 
bytes, which is really dangerous for some systems that support long
user names.

I am not sure how it might affect the program if I don't setup the
current directory of slave processes as the master process. If that
could be a problem, I need to add one more command line option to 
let P4 change to the current directory as the master node. But need
to read the code to see how it locates the procgroup file (in case
it has already considered the path.) This problem might be obvious
when I do a symbolic link to the real executable. (And since I use
realpath() to get the pathname of the executable, the directory part
of the executable might be different from user's real current directory.)
Another problem: suppose I launch a P4 job with dir/prog, will it look
for pg files under current directory or dir? Need to figure out a 
more reasonable specification about it.

P4 internally restricted 256 distinguished machines and 1024 P4 
processes.

argc, argv handling is much safer now. The original P4 has a read
beyond array boundary bug. Also, it might not correctly set the
last argv element to be NULL.
