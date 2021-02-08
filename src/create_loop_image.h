////////////////////////////////
///// create_loop_image.h  /////
////////////////////////////////


#ifndef CREATE_LOOP_IMAGE_HH
#define CREATE_LOOP_IMAGE_HH

#include <kvs/glut/Screen>
#include <kvs/RotationMatrix33>
#include <kvs/ObjectManager>
#include <cstring>

#include "loop_image_param.h"

//----------------//
class sc_loop_image : public kvs::glut::Screen
//----------------//
{
  enum ROT_AXIS { XROTATE, YROTATE, ZROTATE } ;

private:
  double    end_rot_angle  ;
  int       num_rot_images ; 
  ROT_AXIS  rot_axis       ;  
  char      loop_image_name_head[256];

private:
  void setRotAxis ( ROT_AXIS  axis ) { rot_axis = axis ; }

public:

  // Constructor
  sc_loop_image( kvs::glut::Application* application = 0 )
   :kvs::glut::Screen(application)
  {
    setEndRotAngle  ( DEFAULT_END_ROT_ANGLE  );
    setNumRotImages ( DEFAULT_NUM_ROT_IMAGES );
    setRotAxis      ( DEFAULT_ROT_AXIS_NAME  );
    setLoopImageNameHead ( DEFAULT_LOOP_IMAGE_NAME_HEAD );
  }

  // Set functions
  void setEndRotAngle ( double angle )
       { end_rot_angle = angle ; }

  void setNumRotImages( int num )
       { num_rot_images = num; }

  void setRotAxis     ( const char axis[] )
       {
         if (!strncmp( axis, "X", 1) || !strncmp( axis, "x", 1) ) {
           setRotAxis ( XROTATE );
	 } else if (!strncmp( axis, "Y", 1) || !strncmp( axis, "y", 1) ) {
           setRotAxis ( YROTATE );
	 } else if (!strncmp( axis, "Z", 1) || !strncmp( axis, "z", 1) ) {
           setRotAxis ( ZROTATE );
	 } else {
           setRotAxis ( ZROTATE );
	 }
       }

  void setLoopImageNameHead ( const char name_head[] )
       { std::strcpy( loop_image_name_head, name_head ) ; }

  // Loop-image generation
  void paintEvent( void )
  {
    // local variables
    double step_angle = end_rot_angle / (double)num_rot_images;
    kvs::Matrix33f RotMat;    

    // Define a rotation matrix that rotates the scene object
    //  by "step_angle" around the given axis
    if      ( rot_axis == XROTATE )
        RotMat = kvs::Matrix33f::RotationX( step_angle );
    else if ( rot_axis == YROTATE )
        RotMat = kvs::Matrix33f::RotationY( step_angle );
    else
        RotMat = kvs::Matrix33f::RotationZ( step_angle );

    // Loop-image generation
    std::cout << "\nGenerating a series of loop images..." << std::endl;
 
    for ( int i = 0; i < num_rot_images; i++ ) {      
      // Rotate the secne object
      scene()->objectManager()->rotate( RotMat );

      // Get a snapshot image
      kvs::glut::Screen::paintEvent();
      kvs::ColorImage snapshot_image;
      snapshot_image = scene()->camera()->snapshot();

      // Output the image to a file
      std::string o_filename( loop_image_name_head );
      char s[5];
      sprintf( s, "%03d", i );
      o_filename += s;
      o_filename += ".bmp";
      snapshot_image.write( o_filename );
    }
    std::cout << "Done! (PATH: " << loop_image_name_head << "ID.bmp)\n" << std::endl;

    //EndFn: Terminate the program after generating the images. 
    exit(0);

  }//PaingEvent()

};//class sc_loop_image

#endif
