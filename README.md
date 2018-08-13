
This subc compiler are clones of the ones released https://www.t3x.org/subc/index.html.
SubC Compiler, Version 2016-07-26.

According to the website, it's in public domain, meaning no copy rights.

This repository contains modified version of the subc compiler. 
Me and my team's goal is to hack this compiler to do the following things:
 - some 

# -----BUGS---------

## *Quine only works on first two iteration*
 - $ gcc -o test-scc *.c 
 - $ ./test-scc -o more-scc *.c
 - $ ./more-scc -o extreme-scc *.c   --> At this point, it causes error, string quine is not of correct format.
 - Have found the problem, my codes to escape a string works as expected when compiled with gcc compiler, but works UNEXPECTED when compiled with scc-style compiler. Unexpected things such as many things not being escaped, but it does do something, as it increases the str len. SCC SOMEHOW DOESN'e RECOGNIZE double quotes in my codes.

# -----PROGRESS-----

## *Good things to note*
 - FILE Infile is the variable storing the input .c file that is being read into the program (subc).
 - next() in scan.c is key. It is where parsing the input is done, character by character.
 - Include() changes, temporally, what next() is parsing, and then switching back to the original .c being parsed.
 - Each of the subc .c files have at most 17,270 characters (useful for our String variable to be able to store our source code within the program, currently set at size 200,000).
 - Here are the files that have been modified from its default:
	- 1. 'scan.c'
 	- 2. 'prep.c'
 	- 3. 'main.c'
 	- 4. 'decl.c'
 	- 5. 'decl.h'
 	- 6. 'data.h'
  - 7. 'def.h'
	- (Look for '//****************', those are the places where I have added on to the default source code, some default codes are commented out near these indication.)*
 - In a clean subc src file, you must do '$ make all' in terminal, so that some prerequiste file are compiled by gcc before you get the first iteration of scc to compile.
	
## **The idea**
 - Instead of reading character by character from a FILE variable and then compiling, we change it to reading character by charater from a String variable and then compiling.
 - After that, we will now have a way to store our malicious codes within the program and have it compile at ANY time while not leaving any noticable breadcrumbs, except in binary form.
 - From then on, we make our compile source code a quine so that it can reproduce it self in future iterations.

## **What's done (This apply only to Ninja version, I have not documented Breadcrumb version.)**
 - What's currently on github, once compiled, will produce a program is coded to read .c source file as a FILE variable and then be converted to a String variable, character by character. After that, our program will read character by character from the String variable containing our input-source-code, character by character, and then compiling.
  - It's now able to compile it self with no bug.
 - I Have found two places in subc-source-code where FILE Infile is being used or modified, next() in scan.c and Include() in prep.c.
  - next(), originally coded to reading from type FILE, is now edited to read from a String variable.
  - Include(), is purposed to open <include> files and have temporally have FILE Infile points to it, so next() will be parsing any <include> files, and then once that's done, it will point FILE Infile back to the original .c file being parsed.
  - Include() is now edited to work with Strings instead of FILE.
   - *There may be more places where Infiles is being modified, but so far it's working the way it is right now.* 
 - Now will scan all input sources and replace all instances of "Hello" to "Good bye". Codes is in main.c.
 - Maximun string length has been increased to 200,000.
 - Has a string variable to contain the complete source codes of the 7 files that I have modified (specified in section *Good things to note*). 
 - Has a source code that is quine-able, and some for loops to make it a quine.

*Essentially I have modified nothing but the source of input (With a String variable and some support int variables to imitate how type FILE works). The rest of the program will work as usual.*


## **How I imitate FILE using a String**
 - 1. **char \*Insource** for Holding content - *A String variable to hold content of source files instead of using a FILE variable. The difference being that String has the content internally inside the program in the RAM, while FILE points to contents outside the program such as the HDD/SSD and does not retain a copy of the source file inside the program in the RAM.*
 - 2. **int Insourcei** for keeping track - *An index to keep track of what character to parse next in the String, while FILE variable is a pointer and is able to keep track of it self.*
 - 3. **int Insourcelen** for EOF (end of file) signal - *A String length is nesscary to imitate EOF (which is -1 in interger form) in a String, because for some reason, string cannot hold an Ascii value of -1, but a primative type char is able to. So that being said, when the parser hits string length, it will return -1 (EOF).*

 ## **How I made it a quine**
 - 1. String variable, call it S, in main.c that stored main.c in an one line form (Using a preprocess .c file that is included in this github repository).
 - 2. S is then split into two, the first half is to be right after the opening double quote and the second half is to be right before the ending double quote. If the part of the source code containing S is '... s = ""; ...' then S.firsthalf is '... s ="' and S.secondhalf is '"; ...'.
 - 3. Now we find the the index where the half is splited, insert an escape version of S.firsthalf and then an escaped version fo S.secondhalf, so now we have S = S.firsthalf + S.escapedfirsthalf + S.escapedsecondhalf + S.secondhalf.
 - 4. Now If main.c is the input file being compiled, instead of passing the normal main.c to the rest of the program, pass S to the rest of the program.

## **Direction**
 - **Breadcrumbs verison**
  - Clone the github repository
  - $ cd SubCCompilerHack/subc-breadcrumbs/src/
  - $ gcc -o evil-scc *.c
  - Open main.c, comment out line 107->120, 264->272
  - $ ./evil-scc -o infected-scc *.c
  - $ ./infected-scc -o ../../helloworld.exe ../../helloworld.c
  - $ ../../helloworld.exe
 - **Ninja version**
  - Clone the github repository
  - $ cd SubCCompilerHack/subc-ninja/src/
  - $ gcc -o evil-scc *.c
  - $ ./evil-scc -o ../../helloworld.exe ../../helloworld.c
  - $ ../../helloworld.exe

## *More coming soon to your local repository in the end of summer...*
