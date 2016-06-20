## 简易Shell
一个实现了简单Shell功能的程序

## Usage
1.编译程序

```
gcc  shell.c -o myShell
```
2.执行

```
$ .myShell
```
3. 输入支持的相关指令


### 实现的功能　　

#### 1.支持命令参数
* $ echo arg1 arg2 arg3
* $ ls /bin /usr/bin /home  

#### 2.实现内置命令cd、pwd、exit
* $ cd /bin
* $ pwd /bin

#### 3.实现文件重定向
* $ echo hello >log
* $ cat log

```
hello
```

#### 4.实现管道功能
$ cat /etc/passwd | wc -l
#### 5.实现管道和文件重定向


* $ cat input.txt
```
3
2
1
```
* $ cat <input.txt | sort | uniq | cat >output.txt
* $ cat output.txt
```
1
2
3
```

## Developed By
* zh.wen zh.wen@hotmail.com