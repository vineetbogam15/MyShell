CS214 P3 - MyShell 
 - Vineet Bogam (vb422) and Karen Pedri (klp203)

 Key Design Notes + Requirements: 
    - MyShell successfully implements both interactive and batch mode, using isatty() to determine mode and 
        one input loop/parsing algorithm
    - Our program used read() to read in commands from standard input/script, and uses write() to write the 
        printing prompt to stdout in interactive mode
    - MyShell determines the path to the executable file, the list of argument strings through tokenization, and 
        the respective inputs and outputs through redirection checks
    - Wildcards are handled by searching the working files through the respective directory, and adding files 
        to the argument list if they correctly fit the wildcard definition
    - Redirection is handled using dup2() to redirect input/output
    - Pipelines are handled by creating two child processes, one for each side of the pipe
        - We only assume that there is only a maximum of 2 child processes in the inputted command
    - We use a global variable to handle conditions, and check previous commands' exit status
    - Our execution ensures that redirection has precedence over pipelines
    - Both input and output in the same line are accounted for
    - The maximum command length must be 10000 characters (includes the directory path used to search for 
        commands in execute_command())
    - The maximum number of tokens in each command is 1000, where each token length must be less than 1000 characters

Test Plan: 

We split up testing into the different cases presented in the write up: 

Pathnames, bare names, and built-in commands: 
    - We developed basic test inputs without redirection/piping to determine the bare 
        functionality of the program
    - These include: 
        - Built in: pwd, exit, cd test, cd .., which cd, which touch
        - Bare names: touch test, rm test, gcc test.c -o test, mv test newName
        - Pathnames: executing compiled files (EX: ./test, ./a etc.)

Redirection: 
    - ./test > a (Redirect output of test into a)
    - pwd > redirection_check
    - ./test > baz < bar (redirects output of test into baz, which bar uses as input)

Pipeline: 
    - ./test | grep Hello
    - ls | grep *.txt 
    - sort < input.txt | uniq > output.txt
    - cat < input.txt | grep "pattern" > output.txt
    - ls | grep "pattern" < input.txt 


Wildcards: 
    - ls foo*bar.txt

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








