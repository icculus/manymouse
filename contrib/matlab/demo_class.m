%% Example for Matlab's class interface

%  Demo script for the Matlab/Octave MEX wrapper for ManyMouse.
%  Please see the file LICENSE.txt in the source's root directory.
%  Thomas Weibel, 2012/12/20
 

clear all
mm = ManyMouse();

availableMice = mm.init()
driverName = mm.driverName()
deviceNames = mm.deviceName()

i = 0;
while i < 100
    event = mm.pollEvent();
    if ~ strcmp( event.event, 'MANYMOUSE_NO_EVENT' )
       disp( event ); i = i+1;
   end
end
% If this isn't called on Windows before calling 'ManyMouse_Init' again,
% only restarting Matlab will make it work again. So always call
% 'ManyMouse_Quit' on Windows ...
mm.quit();
