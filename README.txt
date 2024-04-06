CS214 P3 - MyShell 
 - Vineet Bogam (vb422) and Karen Pedri (klp203)

 Key Design Notes + Requirements: 
    - MyShell successfully implements both interactive and batch mode, using isatty() to determine mode and 
        one input loop/parsing algorithm
    - Our program uses read() to read in commands from standard input/script, and uses write() to write the 
        printing prompt to stdout in interactive mode
    - MyShell determines the path to the executable file, the list of argument strings through tokenization, and 
        the respective inputs and outputs through redirection and piping checks
    - Wildcards are handled by searching the working files through the working directory, and adding files 
        to the argument list if they correctly fit the wildcard definition
    - Redirection is handled using dup2() to redirect input/output
    - Pipelines are handled by creating two child processes, one for each side of the pipe
        - We assume that there is only a maximum of 2 child processes in the inputted command
    - We use a global variable to handle conditionals, and use them to check previous commands' exit status
    - Our execution ensures that redirection has precedence over pipelines
        - This is because of how execute_command() naturally calls redirection checks in its method
    - Both input and output redirection in the same line are accounted for 
    - We also ensure that (<, >, |) are always considered as tokens through preprocessing before parsing
        - more in comments in mysh.c
    - The maximum command length must be 10000 characters (includes the directory path used to search for 
        commands in execute_command())
    - The maximum number of tokens in each command is 1000, where each token length must be less than 1000 characters

Test Plan: 

We split up testing into the different cases presented in the write up: 
    - we initially needed to test the basic functionality of our program, which 
        consisted of testing our shell can properly execute a singular command
         without special cases. These include pathnames, bare names, and built-in commands.

Pathnames, bare names, and built-in commands: 
    - We developed basic test inputs without redirection/piping to determine the bare 
        functionality of the program
    - These include: 
        - Built in: pwd, exit, cd test, cd .., which cd, which touch
        - Bare names: touch test, rm test, gcc test.c -o test, mv test newName
        - Pathnames: executing compiled files (EX: ./test, ./a etc.)
          - we also ran ./mysh multiple times to create multiple shells inside of each other, 
             and made sure they exitted properly

Next, we tested if our program properly created child processes with dup2() through redirection. 
    - We checked if open() properly created the file if it didn't already exist, as well as opened 
        in mode 0640

Redirection: 
    - ./test > test_output (Redirect output of test into a)
    - pwd > redirection_check
    - ./test > output < input (redirects output of test into output, which should not affect input)

We then ensured that pipes worked, and also made sure that redirection and piping could co-exist
    - Our testing strategy was to create as many combinations of piping and redirection that 
        we could in a logical manner

Pipeline: 
    - ./test | grep Hello
    - ls | grep .txt 
    - sort < input | uniq > output
    - cat < input | grep "pattern" > output
    - ls | grep "pattern" < input
        - input correctly overrides the pipe
    - ls | grep "pattern" > output

Wildcard testing consisted of creating many files of the same wildcard definition in the working directory, 
    and determining of all are listed through the ls linux prompt
    - we assumed that wildcards never followed a redirection symbol, and that multiple can exist in a command

Wildcards: 
    - ls *bar.txt
    - ls *.txt

batchtest.sh 
    - Used to test batch mode/conditionals
    - echo Hello World
        echo Test2
        cd test
        ./test
        cd ..
        cd
        then cd test
        pwd

Another important methodology of testing was testing out our shell against bash, comparing 
    results to ensure that our program was correctly

We also tested most of these commands without whitespace between the redirection and 
    piping symbols to ensure that they were always tokenized







