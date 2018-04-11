If you have not cloned the repository, please input the following command:
```
git clone https://github.com/snowzjx/COMP4621-2018S.git
```
Otherwise, please move to the source repository and pull as follows:
```
git pull
```

Move to directory of lab 3 and compile:
```
cd COMP4621-2018S/lab3
gcc pthread_server.c -lpthread -o pthread_server
gcc server.c -o server
```

To run the program
```
./pthread_server
```

