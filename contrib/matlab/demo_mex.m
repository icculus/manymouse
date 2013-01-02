%% Example for mex function call. Should work both in Octave and Matlab.

%  Demo script for the Matlab/Octave MEX wrapper for ManyMouse.
%  Please see the file LICENSE.txt in the source's root directory.
%  Thomas Weibel, 2012/12/20

clear all
more off;

availableMice = manymouse_mex( 'ManyMouse_Init' )
driverName = manymouse_mex( 'ManyMouse_DriverName' )
deviceNames = manymouse_mex( 'ManyMouse_DeviceName' )

i = 0;
while i < 100
   event = manymouse_mex( 'ManyMouse_PollEvent' );
   if ~ strcmp( event.event, 'MANYMOUSE_NO_EVENT' )
       disp( event ); i = i+1;
   end
end
% If this isn't called on Windows before calling 'ManyMouse_Init' again,
% only restarting Matlab will make it work again. So always call
% 'ManyMouse_Quit' on Windows ...
manymouse_mex( 'ManyMouse_Quit' );
