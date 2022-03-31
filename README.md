# PA1
This reposatory is part of a small project I did in my B.sc degree. the project about computer comunication and sockets operation.

**Comiplation**:
1. Compile via Visual Studio -
- Open project in Visual Studio.
- Build Solution & Run (F5/ctrl + F5).

2. Compile via Developer Command Prompt for VS -
- Open Developer 'Command Prompt for VS' (application file inside 'Visual Studio Tools' directory).
- Go to directory of the wanted C file.
- Type on the Command Prompt : 'cl /EHsc name.c' (name of the file instead of 'name.c')
- Type command for the executable file: 'name argument1 argument2'
* for more information : https://docs.microsoft.com/en-us/cpp/build/walkthrough-compiling-a-native-cpp-program-on-the-command-line?view=msvc-170


**Operation & conditions:**
Operate as described in PA1 + all bonuses.
1. The first must be channel.exe that we will have the ports numbers and ip number. the order doesn't matter between sender.exe and receiver.exe
2. In case of channel failure, both sender and receiver will have connection failure with code: 100
3. To continue to another channel interval answer 'yes' (case-unsensitive), every other answer will stop channel's run.
3. To continue to another sender/receiver interval put in an existing file name to be read, non-valid name will end the run.
4. To end sender/receiver run put in 'quit' (case-unsensitive) when asked for file name.
5. Channel's noise: the noise parameter is in hamming border of success.
