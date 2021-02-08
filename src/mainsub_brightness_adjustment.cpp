//////////////////////////////////////////////////
//   @file   mainsub_brightness_adjustment.cpp
//   @author Tomomasa Uchida
//   @date   2020/09/30
//////////////////////////////////////////////////

#include <kvs/glut/Application>
#include <kvs/Version> //KVS2

#include <kvs/ParticleBasedRenderer> //KVS2

#include <kvs/glut/Screen>

#include <cstring>
#include <iostream>

#include "event_control.h"
#include "file_format.h"
#include "spbr.h"
#include "version.h"
#include "display_opbr_usage.h"

#include "toolxform.h"
#include "shuffle.h"

#include "mainfn_utility.h"

#include "brightness_adjustment.h"
#include <kvs/glut/Timer>

int mainsub_brightness_adjustment(
    kvs::glut::Application*              app,
    int                                  argc,
    char**                               argv,
    SPBR*                                spbr_engine,
    kvs::PointObject*                    object,
    BrightnessAdjustment::FILE_FORMAT4BA file_format )
{
    // Instantiate "BrightnessAdjustment" class
    BrightnessAdjustment* ba = new BrightnessAdjustment( file_format );

    // Create screen
    kvs::glut::Screen screen( app );

    // Forcibly, the background color is set to "Black"
    const kvs::RGBColor bgcolor = spbr_engine->backGroundColor();
    const kvs::RGBColor black   = kvs::RGBColor( 0, 0, 0 );
    screen.setBackgroundColor( black );
    // ba->setBackgroundColor( black );
    if ( bgcolor == black ) { std::cout << "\n";
    } else { std::cout << "*** Forcibly, the background color is set to \"black\".\n" << std::endl; }

    // Register object and renderer
    const unsigned int original_repeat_level = spbr_engine->repeatLevel();
    ba->RegisterObject( screen.scene(), argc, argv, spbr_engine, original_repeat_level );

    // Set camera type (orthogonal/perspective) and 
    //   other camera parameters: 
    //   camera position, look-at position, and view angle
    setCameraParameters( spbr_engine, &screen ); 

    // Set image resolution to the screen
    const unsigned int img_resoln = spbr_engine->imageResolution();
    screen.setGeometry( 0, 0, img_resoln, img_resoln );

    // Set window title
    setWindowTitle( SPBR_WINDOW_TITLE, argv[1], &screen ); 

    // Add initialize event
    InitializeEvent init;
    screen.addEvent( &init );

    // Add keypress event
    KeyPressEvent key;
    screen.addEvent( &key );

    // Add timer event
    const int msec = 1000;
    TimerEvent timer_event(
        /* const int             */  msec,
        /* BrightnessAdjustment* */  ba, 
        /* int                   */  argc, 
        /* char**                */  argv, 
        /* kvs::Scene*           */  screen.scene(), 
        /* SPBR*                 */  spbr_engine, 
        /* const int             */  original_repeat_level );
    screen.addEvent( &timer_event );

    std::cout << "** Executing particle-based rendering...\n" << std::endl;

    // Create and show the window
    screen.show();

    return ( app->run() );
} // End mainsub_brightness_adjustment()
