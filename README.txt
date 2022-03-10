INFO 6016 Network Programming
Evan Sinasac - 1081418

This program was built and compiled in Visual Studio 2019, it only runs in Release mode in x64.

MySQL80 database running at 127.0.0.1 with username "root" and password "root89".
authenticationservice.sql is the dump of the database as of time of submission.
queries.sql is a group of queries I used to test and view the contents of the database.  I would suggest not running more than one or two of them at a time.

Main stuff for project 2 is the DBHelper, and the new commands/messages that can be sent between the client and server.  In particular, if the client sends the server:
/create <email> <password>
the server will attempt to add the account to the database (barring no errors and the account does not already exist), and
/auth <email> <password>
the server will access the database and compare the inputted password and the stored password and salt to see if the email and password are correct.

github link: https://github.com/EvanSinasac/project2networkprogramming.git
github backup: https://github.com/EvanSinasac/NetworkProject2

VIDEO
https://youtu.be/xjCQ8HIgJD8

