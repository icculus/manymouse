classdef ManyMouse

    properties
    end
    
    methods (Static)
        
      
        function availableMice = init(  )
            availableMice = manymouse_mex( 'ManyMouse_Init' );
        end
        
        function quit( )
            manymouse_mex( 'ManyMouse_Quit' );
        end
        
        function sDriverName = driverName(  )
            sDriverName = manymouse_mex( 'ManyMouse_DriverName' );
        end
        
        function sDeviceNames = deviceName(  )
            sDeviceNames = manymouse_mex( 'ManyMouse_DeviceName' );
        end
        
        function event = pollEvent(  )
            event = manymouse_mex( 'ManyMouse_PollEvent' );
        end
        
        
    end
    
end

