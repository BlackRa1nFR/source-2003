Written by Russ Freeman Copyright ©1998-2001.
Email: russf@gipsysoft.com
Web site: http://www.gipsysoft.com/
Last updated: June 30, 2001

Sample project that shows how to build QHTM statically linked to an executable.

The process is fairly simple:
1) Add all QHTM files except DLLMain.cpp to your project.
2) Add the resources noimage.bmp and hand.ico
3) Add a call to QHTM_SetResources(...) in your initialisation to set the "hand" and "no image" resources as well as the resource handle to use.
4) Add calls to QHTM_Initialize(...), and optionally QHTM_EnableCooltips(...), to your initialisation.

*** Build!
