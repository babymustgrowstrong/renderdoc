This fork is made to make Android development using Visual Studio easier.<br>

A few known issues while using an opengl es 3.1 phone:<br>
1. Post-VS vertices is disabled currently because of the lack of transform feedback extension.<br> 
2. D24 depth targets don't transfer back properly.<br>
3. Some textures don't transfer properly.<br>


<b>To use renderdoc for gearvr:</b><br>

First, make a build:<br>
1. Run \android-build\make_android.bat<br>
2. Open \android-build\RenderDoc.sln in visual studio and then build solution<br>
3. This generates \android-build\bin\RenderDocCmd.apk and \android-build\bin\Debug\librenderdoc.so (librenderdoc.so is the library hook you need to link with your application).<br>
4. Install RenderDocCmd.apk with this command: adb install RenderDocCmd.apk<br>

Then, link against librenderdoc.so:<br>
5. Add the linker option of -lrenderdoc<br>
6. make sure your android application has these 2 permissions: android.permission.WRITE_EXTERNAL_STORAGE for writing the capture file and android.permission.INTERNET for connecting to the windows client.<br>
7. make sure to add librenderdoc.so as part of your android packaging step so it gets included as part of the apk.<br>

Finally, run your application with adb connected:<br>
8. You can have your phone connected through adb with an USB connection, or through wifi.<br>
9. See https://www.google.com/search?q=connecting+adb+over+wifi for instruction to connect adb over wifi.<br>

To make a capture, run renderdoc from windows:<br>
10. Under Tools>Options, provide the path to your adb.exe in the Android section.<br>
11. Click File>Attach to Running Instance, your application should be listed.<br>
12. If you are connecting through wifi, you will need to enter your phone's IP into the field under "Hostname:", then click "Add"<br>
13. After you are connected, if the vrapi hooks are working, there should be a message that looks like this: Connection established to &lt;app package name&gt;[PID &lt;process id&gt;]<b>(GearVR)</b><br>
14. Click "Trigger Capture" to trigger a capture, you should get a captured frame represented as a screenshot (currently black because screenshot is not working yet)<br>

To replay the capture:<br>
15. Click Tools>Start Android Remote Server, RenderDocCmd should start on your phone.<br>
16. Back to the RenderDoc windows client, in the lower left corner there is a text "Replay Context:", "Local" should be selected by default. Click on it and you will be presented with a list of other options. If you connected to your phone through wifi, your phone's ip will be listed. If you connected to your phone through USB, a MAC address like number will be listed. Click on the option that represent your phone.<br>
17. Finally, double click on your screen and your capture should replay over your phone.<br>
