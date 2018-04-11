ource repository.
```
git clone https://github.com/snowzjx/COMP4621-2018S.git
```

Move to directory of lab 2 solution and compile:
```
cd COMP4621-2018S/lab2_solution
gcc client.c -o client
gcc server.c -o server
```

Run server first
```
./server
```

Then run client to fetch file from server
```
./client [filename]
```

If the file exist, you will get the content of the file
```
./client 1.txt
Hello. This is 1.txt.
```

If the file does not exist, you will get an error message
```
./client 2.txt
The file your requested does not exist ...
```
