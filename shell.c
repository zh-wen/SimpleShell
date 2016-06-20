/*************************************************************************
    > File Name: sh3.c
    > Author: zh.wen
    > Mail: zh.wen@hotmail.com 
    > Created Time: 2016年06月26日 星期日 18时32分20秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>  
#include <signal.h>


int STDOUT; //保存标准输出的文件描述符
int STDIN;	//保存标准输入的文件描述符

 //argument buff
char *argbuff[20];
int argcnt; 
char cur_dir[100];
//内置命令
char built_in_command[][20] = {"cd","pwd","exit"};

//判断是否是内置命令
int is_builtin_cmd(char *cmd)
{
	int i ;
	for(i = 0; i < 3; i++)
	{
		if(strcmp(cmd,built_in_command[i]) == 0)
		{
			return 1;
		}
	}
	return 0;
}

//实现echo功能
int do_echo(int argc, char *argv[])
{
	int i ;
	for(i = 0; i < argc; i++)
	{
		if(i > 0)
		{
			write(STDOUT_FILENO," ",1);
		}
		write(STDOUT_FILENO,argv[i],sizeof(argv[i]));
	};
	write(STDOUT_FILENO,"\n",1);
}

//添加参数
void add_arg(char *buff)
{
	argbuff[argcnt] = (char*)malloc(strlen(buff)+1);
	strcpy(argbuff[argcnt],buff);
	argcnt++;
}

//释放存放参数的空间
void free_arg()
{
	int i;
	for(i = 0; i < argcnt; i++)
	{
		free(argbuff[i]);
		argbuff[i] = NULL;
	}
}

//处理参数
void parse_arg(char *chbuff)
{
	char *p = chbuff;
	char buff[50];
	int i = 0;
	while(*p)
	{
		//若 当前符号为 '<','>','|'，则作为单独参数
		if((i==0)&&((*p =='<')||(*p=='>')||(*p =='|')))
		{
			buff[i++] = *p;
			buff[i] = '\0';
			add_arg(buff);
			i = 0;
		}
		else if((*p != ' ') &&(*p !='\t')&&(*p !='\n'))
		{
			buff[i] = *p;
			i++;
		}		
		//遇到空格或换行，制表符，添加参数	
		else 
		{
			buff[i] = '\0';
			if(i != 0)
			{
				add_arg(buff);
			}
			i = 0;
		}
		p++;
	}
	if(i != 0)
	{
		buff[i] = '\0';
		add_arg(buff);
	}
}


//显示文件名
void display_file(char *dirname)
{
	DIR *dir_ptr;
	struct dirent *direntp;
	char full_path[256];
	if((dir_ptr = opendir(dirname)) == NULL)
	{
		fprintf(stderr, "ls: cannot open%s\n",dirname );
	}
	else
	{
		while((direntp = readdir(dir_ptr)) != NULL)
		{
			printf("%s\t",direntp->d_name);
		}
		printf("\n");
		closedir(dir_ptr);
			
	}
}

//实现ls功能
int do_ls(int argc, char **argv)
{
	int i ;
	if(argc == 0)
	{
		display_file(".");
	}
	else
	{
		for(i = 0; i < argc; i++ )
		{
			printf("%s:\n",argv[i]);
			display_file(argv[i]);	
		}
	}
	return 0;
}

//实现exit功能
int do_exit(int argc, char **argv)
{
	int value = 0;
	if(argc > 0)
	{
		value = atoi(argv[1]);
	}
	exit(value);
	return 0;
}

//实现cd功能
int do_cd(int argc, char **argv)
{
	if(argc == 0)
	{
		return -1;
	}
	else
	{
		if(chdir(argv[0]) < 0)
		{
			perror("chdir failed\n");
			return -1;
		}

	}
}

//实现pwd功能
int do_pwd(int argc, char **argv)
{
	getcwd(cur_dir,100);
	printf("%s\n",cur_dir);
}

//处理重定向
int predo_for_redirect(int argc,char **argv,int *re)
{
	int i;
	int redirect = 0;
	//扫描参数，看是否有重定向符号
	for( i = 1 ; i < argc; i++)
	{
		if(strcmp(argv[i],"<") == 0)
		{
			redirect = 1;
			free(argv[i]);
			argv[i] = 0;
			break;
		}
		else if(strcmp(argv[i],">") == 0)
		{
			redirect = 2;
			free(argv[i]);
			argv[i] = 0;
			break;
		}
	}

	// 若有重定向符号
	if(redirect)
	{
		if(argv[i+1]) 
		{
			int fd;
			//重定向标准输出
			if(redirect == 2)  
			{
				if((fd = open(argv[i+1], O_WRONLY|O_CREAT|O_TRUNC, 
					S_IRUSR|S_IWUSR)) == -1) 
				{
					fprintf(stderr, "Open out %s failed\n", argv[i+1]);
					return 1;
				}
				STDOUT = dup(STDOUT_FILENO);
				dup2(fd, STDOUT_FILENO);
				close(fd);
				free(argv[i+1]);
				argv[i+1] = 0;
			} 
			//重定向标准输入
			else 
			{
				if((fd = open(argv[i+1], O_RDONLY, S_IRUSR|S_IWUSR)) == -1)
				 {
					fprintf(stderr, "Open in %s failed\n", argv[i+1]);
					return 1;
				}
				STDIN = dup(STDIN_FILENO);
				dup2(fd, STDIN_FILENO);
				close(fd);
				free(argv[i+1]);
				argv[i+1] = 0;
			}
		} 
		else 
		{
			fprintf(stderr, "Bad redirect, need more arg\n");
			return 1;
		}
	}
	if(re)
		*re = redirect;
	return 0;
}

//恢复重定向
void reset_redirect()
{
	dup2(STDIN, STDIN_FILENO);
	dup2(STDOUT, STDOUT_FILENO);
}

//执行内部命令或外部命令
void select_cmd(int argc, char** argv,int argn)
{
	if(strcmp(argbuff[0],"ls") == 0)
	{
  	 	do_ls(argc - argn ,&argv[1]);
	}
	else if(strcmp(argbuff[0],"echo") == 0)
	{
		do_echo(argc - argn,&argv[1]);
	}
	else if(strcmp(argbuff[0],"cd") == 0)
	{
		do_cd(argc - argn,&argv[1]);
	}
	else if(strcmp(argbuff[0],"pwd") == 0)
	{
		do_pwd(argc - argn,&argv[1]);
	}
	else if(strcmp(argbuff[0],"exit") ==0)
	{
		do_exit(argc - argn, &argv[1]);
	}
	else if(strcmp(argbuff[0],"cat") == 0)
	{
		do_cat(argc - argn, &argv[1]);
	}
	else if(execvp(argv[0],argv) < 0)
	{
		printf("%s : command not found\n",argv[0]);
	}
}

//执行简单的命令
int do_single_cmd(int argc, char ** argv, int *prefd, int *postfd)
{
	int pid;
	int status;
	int have_redirect = 0;
	int argn = 1;

	if(argc == 0)
	{
		return 0;
	}
	//如果没有管道
	if(prefd == 0 && postfd == 0)
	{
		//判断是否是内置命令，父进程执行内置命令
		if(is_builtin_cmd(argv[0]))
		{
			if(predo_for_redirect(argc,argv,&have_redirect))
			{	
				return 1;
			}

			if(have_redirect)
			{	
				argn = 3;
			}
			select_cmd(argc , argv,argn);
			if(have_redirect)
			{
				reset_redirect();
			}
			return 0;
		}
	}

	//创建子进程，除了没有管道的简单内置命令
	//其余命令都在子进程中执行
	if((pid = fork()) == 0)
	{
		have_redirect = 0;
		if(predo_for_redirect(argc,argv,&have_redirect))
		{	
			exit(1);
		}
		//没有重定向标准输入，且有前管道
		//重定向标准输入
		if(have_redirect !=1 && prefd)
		{
			close(prefd[1]);
			if(prefd[0] != STDIN_FILENO) {
				dup2(prefd[0], STDIN_FILENO);
				close(prefd[0]);
			}
		}
		//没有重定向标准输出，且有后管道
		//重定向标准输出
		if(have_redirect != 2 && postfd)
		{
			close(postfd[0]);
			if(postfd[1] != STDOUT_FILENO)
			{
				dup2(postfd[1], STDOUT_FILENO);
				close(postfd[1]);
			}
		}

		//若有重定向，则删除重定向符号和文件名
		//删除最后两个参数
		if(have_redirect)
		{	
			argn = 3;
		}
		select_cmd(argc,argv,argn);
		exit(0);
	}
	//等待子进程结束
	waitpid(pid,&status,0);
	if(postfd)
	{
		close(postfd[1]);
	}
	return 0;
}

//实现cat功能
int do_cat(int argc, char **argv)
{
	char ch;
	FILE *fp;
	//不带参数
	if(argc == 0)
	{
		while((ch= getc(stdin)) != EOF)
		{
			putc(ch,stdout);
		}
	}
	//带多个参数
	else
	{
		while(argc-- > 0)
		{
			if((fp = fopen(*argv++, "r")) == NULL)
			{
				printf("cat:can't  open %s\n", *argv);
				return 1;
			}
			else
			{
				while((ch= getc(fp)) != EOF)
				{
					putc(ch,stdout);
				}
				fclose(fp);
			}
		}
	}
	return 0;
}

//实现管道功能
int do_pipe_cmd(int argc, char** argv)
{
	int i = 0;
	int j = 0;
	int prepipe = 0;
	int prefd[2]; //前管道
	int postfd[2]; //后管道
	char* p;

	//扫描参数，查看是否有"|"
	while(argv[i])
	 {
	 	//有管道
		if(strcmp(argv[i], "|") == 0)
		 { 
			p = argv[i];
			argv[i] = 0;

			//创建管道
			pipe(postfd); 		
			//有前管道,且有后管道
			if(prepipe)	
				do_single_cmd(i-j, argv+j, prefd, postfd);
			//有后管道，没有前管道
			else
				do_single_cmd(i-j, argv+j, 0, postfd);
			argv[i] = p;
			prepipe = 1;
			prefd[0] = postfd[0];
			prefd[1] = postfd[1];
			j = ++i;
		} else
			i++;
	}
	//有前管道
	if(prepipe)
		do_single_cmd(i-j, argv+j, prefd, 0);
	else 
	//没有前管道
		do_single_cmd(i-j, argv+j, 0, 0);
	return 0;
}

int main()
{
	char chbuff[100];
	int i;
	while(1)
	{ 
		printf("zh.wen  shell:");
		getcwd(cur_dir,100);
		printf("%s$ ",cur_dir);
		//读取命令行
		gets(chbuff);
		//获取参数
		free_arg();
		argcnt =  0;
		parse_arg(chbuff);
		//执行命令
		do_pipe_cmd(argcnt,argbuff);
	}	

	return 0;
}
