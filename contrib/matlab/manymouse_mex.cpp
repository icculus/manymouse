/*
 * A Matlab / Octave MEX wrapper for ManyMouse.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  Thomas Weibel, 2012/12/20
 */

#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "manymouse.h"

#include <mex.h>
#include <stdint.h>


// Convert mxArray* to an std::string
void convert( const mxArray* ma, std::string& sString )
{
    if( !mxIsChar( ma ) )
        mexErrMsgTxt( "!mxIsChar" );
    char *str = mxArrayToString( ma );
    sString = std::string( str );
}

//! Converts a double (or int, ...) scalar to mxArray
mxArray* convert( const double& scalar, mxArray*& ma )
{
    ma = mxCreateDoubleScalar( (double) scalar );
    return ma;
}

// MEX gateway function
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    if( nrhs != 1 )
        mexErrMsgTxt( "manymouse_mex: nrhs != 1.\n" );
    
    std::string sMethod;
    convert( prhs[0], sMethod );
    static ManyMouseEvent event;
    static int available_mice;
    
    if( sMethod == "ManyMouse_Init" )
    {
        ManyMouse_Quit(); 
        available_mice = ManyMouse_Init();
        if( available_mice == 0 )
        {
            ManyMouse_Quit();
            mexErrMsgTxt( "manymouse_mex: Found no mice ...\n" );
        }
        else if (available_mice < 0)
        {
            ManyMouse_Quit();
            mexErrMsgTxt( "Error initializing ManyMouse!\n" );          
        }        
        convert( available_mice, plhs[0] );
    }
    else if( sMethod == "ManyMouse_Quit" )
    {
        ManyMouse_Quit(); 
    }
    else if( sMethod == "ManyMouse_DriverName" )
    {
        plhs[0] = mxCreateString(  ManyMouse_DriverName() );
    }
    else if( sMethod == "ManyMouse_DeviceName" )
    {
        plhs[0] = mxCreateCellMatrix( available_mice, 1 );
        for (int i = 0; i < available_mice; i++)
            mxSetCell( plhs[0], i, mxCreateString( ManyMouse_DeviceName(i) ) );
    }
    else if( sMethod == "ManyMouse_PollEvent" )
    {            
        if( ManyMouse_PollEvent(&event) )
        {
            const char* fieldnames[] = {"device", "event", "item", "value" };
            plhs[0] = mxCreateStructMatrix(1,1,4,fieldnames);
        
            mxSetField( plhs[0], 0, "device", mxCreateDoubleScalar( (double) event.device ) );
            // Could also return item as integer ...
            //mxSetField( plhs[0], 0, "event", mxCreateDoubleScalar( (double) event.type ) );
            mxSetField( plhs[0], 0, "item", mxCreateDoubleScalar( (double) event.item ) );
            mxSetField( plhs[0], 0, "value", mxCreateDoubleScalar( (double) event.value ) );
            
            if (event.type == MANYMOUSE_EVENT_RELMOTION)
            {
                mxSetField( plhs[0], 0, "event", mxCreateString( "MANYMOUSE_EVENT_RELMOTION" ) );        
            }
            else if (event.type == MANYMOUSE_EVENT_ABSMOTION)
            {
                mxSetField( plhs[0], 0, "event", mxCreateString( "MANYMOUSE_EVENT_ABSMOTION" ) );        
            }
            else if (event.type == MANYMOUSE_EVENT_BUTTON)
            {
                mxSetField( plhs[0], 0, "event", mxCreateString( "MANYMOUSE_EVENT_BUTTON" ) );
            }
            else if (event.type == MANYMOUSE_EVENT_SCROLL)
            {
                mxSetField( plhs[0], 0, "event", mxCreateString( "MANYMOUSE_EVENT_SCROLL" ) );            
            }
            else if (event.type == MANYMOUSE_EVENT_DISCONNECT)
                mxSetField( plhs[0], 0, "event", mxCreateString( "MANYMOUSE_EVENT_DISCONNECT" ) ); 
            else
            {
                mxSetField( plhs[0], 0, "event", mxCreateString( "MANYMOUSE_UNHANDLED_EVENT" ) ); 
            }     
        }   
        else
        {
            const char* fieldnames[] = {"device", "event", "item", "value" };
            plhs[0] = mxCreateStructMatrix(1,1,4,fieldnames);     
            mxSetField( plhs[0], 0, "device", mxCreateDoubleScalar( -1 ) );
            mxSetField( plhs[0], 0, "event", mxCreateString( "MANYMOUSE_NO_EVENT" ) ); 
            mxSetField( plhs[0], 0, "item", mxCreateDoubleScalar( -1 ) );
            mxSetField( plhs[0], 0, "value", mxCreateDoubleScalar( -1 ) );
        }
    }   
    else
        mexErrMsgTxt( "manymouse_mex: Not a supported method." );    
}

