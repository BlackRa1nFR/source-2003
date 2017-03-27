Written by Russell Freeman Copyright ©1998.
Email: debughlp@gipsysoft.com
Web site: http://www.gipsysoft.com/
Last updated: January 22, 1999, Version 1.1

Want an update for this? Then check out the web site, if there has
been an update then you can find it there.

This project provides some essential debug helper functions and
macros based on those available with MFC and the C runtime but
with some handy extensions.

Why Use DebugHlp.
=================
I like my code to be as robust as possible, I like my code to
shout out whenever I call a function incorrectly, I want as much
help avoiding bugs possible. Assertions in your code assure you
that your program is healthy, DebugHlp provides assertions, trace
output and other helpful macros.

DebugHlp adds several new ideas to the standard MFC and runtime
assertions and trace output.

* The filename and line number of the trace statement is output
  as well as the trace text, this means that when a trace statement
	is viewed in the Microsoft Visual C++ debugger you can simply double
	click on the text in the output window to go to the exact soruce line
	of the trace statement.
* ASSERT also puts the expression in the Abort, Retry, Ignore dialog box.
  The MFC ASSERT does not. The runtime version does but when you Retry it
	dumps you in the assertion code not yours - you need to crawl up the
	stack to get to your code. DebugHlp is the best of both.
* When an assert fails the expression is traced out. This means you can
  Ignore the assertion failures knowing that the line and filename of the
	offending ASSERT has been recorded in the output window.
* Handy VERIFY extension will also display the error code and the matching
  error string, no more guessing why an API failed. Includes being copied
	to the output window.
* Extra assertions to assert the validity of a HWND, whether a string is
  valid and whether a pointer is readable/writeable.

If you code using API and you like using ASSERTs and VERIFYs then DebugHlp
will lighten the load. If you use MFC then DebugHlp adds some welcome
extension.

Legal
=====
This notice must remain intact.
The files belong wholly to Russell Freeman.
You may use the files compiled in your applications.
You may not sell the files in source form.
This source code may not be distributed as part of a library,

I think we all understand that I don't want the library sold
for profit or merged in with someone elses library, nor do I
want you claiming it as your own. Other than that you can use
the code to help you create solid applications and as it is a
debug helper library you will not really want to distribute it
with your applications.

History
=======
* Fixed problems with including the project in a console app.