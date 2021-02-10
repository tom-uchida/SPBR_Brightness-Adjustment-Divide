//////////////////////////////////////////
//   @file   brightness_adjustment.cpp
//   @author Tomomasa Uchida
//   @date   2020/09/30
//////////////////////////////////////////

#include <kvs/glut/Application>
#include <kvs/Version> //KVS2

#include <kvs/PointObject>

#include <kvs/ParticleBasedRenderer> //KVS2

#include <kvs/glut/Screen>
#include <kvs/Camera>
#include <kvs/RotationMatrix33>

#include <cstring>
#include <iostream>

#include "single_inputfile.h"
#include "file_format.h"
#include "spbr.h"
#include "mainfn_utility.h"
#include "shuffle.h"

#include "brightness_adjustment.h"
#include <kvs/ColorImage>
#include <kvs/GrayImage>

#include "version.h"
#include <sstream>

#include <time.h>

const float PERCENT_IN_REFERENCE_SECTION = 0.01f;
const float PARAMETER_INTERVAL = 0.01f;

// Constructor
BrightnessAdjustment::BrightnessAdjustment( 
    const FILE_FORMAT4BA file_format,
    const int id
):
    m_file_format( file_format ),
    m_adjustment_type( OFF ),
    m_bgcolor( kvs::RGBColor( 0, 0, 0 ) ),
    m_snapshot_counter( 0 )
{
    std::cout << "\n*** BrightnessAdjustment constructor is called:\n";

    // Display the file format of the input data
    if ( m_file_format == PLY_ASCII4BA )
        std::cout << "      FILE_FORMAT: PLY_ASCII\n";
    else if ( m_file_format == PLY_BINARY4BA )
        std::cout << "      FILE_FORMAT: PLY_BINARY\n";
    else if ( m_file_format == SPBR_ASCII4BA )
        std::cout << "      FILE_FORMAT: SPBR_ASCII\n"; 
    else if ( m_file_format == SPBR_BINARY4BA )
        std::cout << "      FILE_FORMAT: SPBR_BINARY\n";
    // end if

    // Set the adjustment type
    if ( id == 1 ) {
        m_adjustment_type = UNIFORM;
        std::cout << "      ADJUSTMENT_TYPE: UNIFORM\n";
    
    } else if ( id == 2 ) {
        m_adjustment_type = DIVIDE;
        std::cout << "      ADJUSTMENT_TYPE: DIVIDE\n";
    } // end if

} // End constructor

void BrightnessAdjustment::RegisterObject( kvs::Scene* scene, int argc, char** argv, SPBR* spbr_engine, const size_t repeat_level )
{
    scene->registerObject( CreateObject( argc, argv ), CreateRenderer( spbr_engine, repeat_level ) );
} // End RegisterObject()

kvs::PointObject* BrightnessAdjustment::CreateObject( int argc, char** argv ) {
    kvs::PointObject* object = NULL;

    // Read the first data file (argv[1])
    if ( m_file_format == PLY_ASCII4BA ) {
        SPBR* spbr_engine = new SPBR( argv[1], PLY_ASCII );
        object = CreateObjectCommon( argc, argv, spbr_engine );

    } else if ( m_file_format == PLY_BINARY4BA ) {
        SPBR* spbr_engine = new SPBR( argv[1], PLY_BINARY );
        object = CreateObjectCommon( argc, argv, spbr_engine );

    } else if ( m_file_format == SPBR_BINARY4BA ) {
        SPBR* spbr_engine = new SPBR( argv[1], SPBR_BINARY );
        object = CreateObjectCommon( argc, argv, spbr_engine );

    } else if ( m_file_format == SPBR_ASCII4BA ) {
        SPBR* spbr_engine = new SPBR( argv[1], SPBR_ASCII );
        object = CreateObjectCommon( argc, argv, spbr_engine );
    } // end if

    return object;
} // End CreateObject()

kvs::PointObject* BrightnessAdjustment::CreateObjectCommon( int argc, char** argv, SPBR* spbr_engine ) {
    kvs::PointObject* object = spbr_engine;

    // Read and append the remaining files:  
    //  argv[2], argv[3], ..., argv[argc-1]
    for ( int i = 3; i <= argc; i++ ) {
        if ( isASCII_PLY_File( argv[i - 1] ) ) {
            SPBR* spbr_tmp = new SPBR( argv[i - 1], PLY_ASCII );
            object->add( *kvs::PointObject::DownCast( spbr_tmp ) );

        } else if ( isBINARY_PLY_File( argv[i - 1] ) ) {
            SPBR* spbr_tmp = new SPBR( argv[i - 1], PLY_BINARY );
            object->add( *kvs::PointObject::DownCast( spbr_tmp ) );
        
        } else if ( isBinarySPBR_File( argv[i - 1] ) ) {
            SPBR* spbr_tmp = new SPBR( argv[i - 1], SPBR_BINARY );
            object->add( *kvs::PointObject::DownCast( spbr_tmp ) );
        
        } else {
            SPBR* spbr_tmp = new SPBR( argv[i - 1], SPBR_ASCII );
            object->add( *kvs::PointObject::DownCast( spbr_tmp ) );
        } // end if
    } // end for

    addBoundingBoxToScene( spbr_engine );

    // Forced shuffle
    if ( spbr_engine->isForcedShuffleOn() ) {
        Shuffle shuffle_engine( spbr_engine );
    }

    // Set object name
    object->setName( "Object" );

    // Object rotation (Z==>X) if required
    if ( spbr_engine->isZXRotation() ) {
        double zrot_deg = spbr_engine->objectZXRotAngle(0);
        double xrot_deg = spbr_engine->objectZXRotAngle(1);
        ToolXform::rotateZX( object, zrot_deg, xrot_deg, kvs::Vector3f( 0, 0, 0 ) );
    }

    return object;
} // End CreateObject()

kvs::glsl::ParticleBasedRenderer* BrightnessAdjustment::CreateRenderer( SPBR* spbr_engine, const size_t repeat_level )
{
    kvs::glsl::ParticleBasedRenderer* renderer = new kvs::glsl::ParticleBasedRenderer();

    // Set rendere name
    renderer->setName( "Renderer" );

    // Set repeat level
    renderer->setRepetitionLevel( repeat_level );
    
    // Set Lambert shading or keep Phong shading
    setShadingType( spbr_engine, renderer );

    // Shading control (ON/OFF)
    if ( spbr_engine->isShading() == false ) {
        std::cout << "** Shading is off" << std::endl;
        renderer->disableShading();
    }

    // LOD control (ON/OFF)
    if ( spbr_engine->isLOD() )
        renderer->enableLODControl();

    // Particle zoom control (ON/OFF)
    if ( spbr_engine->isParticleZoomOn() == false )
        renderer->disableZooming();
    else
        renderer->enableZooming();

    // Shuffle control (ON/OFF)
    if ( spbr_engine->isParticleShuffleOn() )
        renderer->enableShuffle();
    else
        renderer->disableShuffle();

    return renderer;
} // End CreateRenderer()

void BrightnessAdjustment::ReplaceObject( kvs::Scene* scene, int argc, char** argv, SPBR* spbr_engine, const size_t repeat_level )
{
    scene->replaceObject( "Object", CreateObject( argc, argv ) );
    scene->replaceRenderer( "Renderer", CreateRenderer( spbr_engine, repeat_level ) );
} // End ReplaceObject()

void BrightnessAdjustment::SnapshotImage( kvs::Scene* scene, const std::string filename, const int repeat_level )
{
    // Snapshot
    scene->screen()->redraw();
    kvs::ColorImage color_image_tmp = scene->camera()->snapshot();

    // Save the snapshot image
    if ( m_snapshot_counter == 0 ) m_color_image     = color_image_tmp;
    if ( m_snapshot_counter == 1 ) m_color_image_LR1 = color_image_tmp;

    // Write the snapshot image
    // color_image_tmp.write( filename + "_LR" + kvs::String::ToString( repeat_level ) + ".bmp" );

    // Update snapshot counter
    m_snapshot_counter++;
} // End SnapshotImage()

void BrightnessAdjustment::AdjustBrightnessUniformVersion( const std::string filename )
{
    // Display opening message
    displayMessage();

    // Calc number of pixels of the image
    const size_t npixels             = m_color_image.numberOfPixels();
    const size_t npixels_non_bgcolor = calcNumberOfPixelsNonBGColor( m_color_image );
    std::cout   << "*** Number of pixels                  : " 
                << npixels             << " (pixels)" << std::endl;
    std::cout   << "*** Number of pixels non-BGColor      : " 
                << npixels_non_bgcolor << " (pixels)" << std::endl;

    // Convert color to gray
    const kvs::GrayImage gray_image( m_color_image );
    const kvs::GrayImage gray_image_LR1( m_color_image_LR1 );

    // ===================================
    //  STEP1: Get max pixel value (LR=1)
    // ===================================
    const clock_t start = clock();
    const kvs::UInt8 max_pixel_value     = calcMaxPixelValue( gray_image );
    const kvs::UInt8 max_pixel_value_LR1 = calcMaxPixelValue( gray_image_LR1 );
    std::cout   << "*** Max pixel value                   : " 
                << +max_pixel_value << " (pixel value)"     << std::endl;
    std::cout   << "*** Max pixel value (LR=1)            : " 
                << +max_pixel_value_LR1 << " (pixel value)" << std::endl;

    // ================================================
    //  STEP2: Search for threshold pixel value (LR=1) 
    // ================================================
    const size_t npixels_non_bgcolor_LR1 = calcNumberOfPixelsNonBGColor( m_color_image_LR1 );
    const kvs::UInt8 threshold_pixel_value_LR1 = searchThresholdPixelValue( gray_image_LR1, npixels_non_bgcolor_LR1, max_pixel_value_LR1 );

    // =======================================
    //  STEP3: Adjust brightness of the image
    // =======================================
    std::cout   << "\n*** Executing \"Brightness Adjustment\"..." << std::endl;
    float p = calcAdjustmentParameter( m_color_image, threshold_pixel_value_LR1, npixels_non_bgcolor );
    p = specifyNumberOfDigits( p, 4 );
    execBrightnessAdjustment( m_color_image, p );

    const clock_t end = clock();
    std::cout   << "*** Done! ( " 
                << static_cast<double>( end - start ) / CLOCKS_PER_SEC << " [sec] )\n" << std::endl;

    std::cout   << "*** Adjustment parameter              : " 
                << std::setprecision(3) << p << std::endl;

    const float percent_final = calcFinalPercent( m_color_image, threshold_pixel_value_LR1, npixels_non_bgcolor );
    std::cout   << "*** Percent in reference section      : " 
                << std::setprecision(3) << percent_final*100 << "(%) ( " 
                << +threshold_pixel_value_LR1 << " ~ " << +max_pixel_value_LR1 << " (pixel value) )" << std::endl;

    // Write adjusted image
    writeAdjustedImage( filename, m_color_image, p );

} // End AdjustBrightness()

inline void BrightnessAdjustment::displayMessage() const
{
    std::cout << "\n\n";

    if ( m_adjustment_type == UNIFORM ) 
        std::cout << BA_UNIFORM_TITLE << std::endl;
    else if ( m_adjustment_type == DIVIDE )
        std::cout << BA_DIVIDE_TITLE  << std::endl;

    std::cout << "\n";
} // End displayMessage()

int BrightnessAdjustment::calcNumberOfPixelsNonBGColor( const kvs::ColorImage& color_image ) const
{
    size_t non_bgcolor_counter = 0;

    for ( size_t j = 0; j < color_image.height(); j++ ) {
        for ( size_t i = 0; i < color_image.width(); i++ ) {
            if ( color_image.pixel( i, j ) == m_bgcolor ) {
            } else { non_bgcolor_counter++; }
        }
    }

    return non_bgcolor_counter;
} // End calcNumOfPixelsNonBGColor()

kvs::UInt8 BrightnessAdjustment::calcMaxPixelValue( const kvs::GrayImage& gray_image ) const
{
    kvs::UInt8 max_pixel_value = 0;

    for ( size_t j = 0; j < gray_image.height(); j++ )
        for ( size_t i = 0; i < gray_image.width(); i++ )
            if ( gray_image.pixel( i, j ) > max_pixel_value ) 
                max_pixel_value = gray_image.pixel( i, j );

    return max_pixel_value;
} // End calcMaxPixelValue()

kvs::UInt8 BrightnessAdjustment::searchThresholdPixelValue( const kvs::GrayImage& gray_image, const size_t npixels_non_bgcolor_LR1, const kvs::UInt8 max_pixel_value_LR1 ) const
{
    kvs::UInt8 threshold_pixel_value_LR1 = max_pixel_value_LR1;
    float percent_tmp = 0.0f;

    // Search for threshold pixel value
    while ( percent_tmp < PERCENT_IN_REFERENCE_SECTION ) {
        int counter = 0;
        for ( size_t j = 0; j < gray_image.height(); j++ )
            for ( size_t i = 0; i < gray_image.width(); i++ )
                if ( gray_image.pixel( i, j ) >= threshold_pixel_value_LR1 ) 
                    counter++;

        percent_tmp = float( counter ) / float( npixels_non_bgcolor_LR1 );
        
        // Next pixel value
        threshold_pixel_value_LR1--;
    } // end while
    threshold_pixel_value_LR1++;

    std::cout   << "*** Threshold pixel value (LR=1)      : " 
                << +threshold_pixel_value_LR1 << " (pixel value)" << std::endl;
    std::cout   << "*** Percent in ref. section (LR=1)    : " 
                << std::setprecision(3) << percent_tmp*100 << "(%) ( " 
                << +threshold_pixel_value_LR1 << " ~ " << +max_pixel_value_LR1 << " (pixel value) )" << std::endl;

    return threshold_pixel_value_LR1;
} // End searchThresholdPixelValue()

float BrightnessAdjustment::calcAdjustmentParameter( const kvs::ColorImage& color_image, const kvs::UInt8 threshold_pixel_value_LR1, const size_t npixels_non_bgcolor )
{
    float adjustment_parameter = 1.0f;
    float percent_tmp = 0.0f;
    
    while ( percent_tmp < PERCENT_IN_REFERENCE_SECTION ) {
        // Update adjustment parameter
        adjustment_parameter += PARAMETER_INTERVAL;

        percent_tmp = calcTemporaryPercent(
            /* kvs::ColorImage  */ color_image, 
            /* const float      */ adjustment_parameter, 
            /* const kvs::UInt8 */ threshold_pixel_value_LR1, 
            /* const size_t     */ npixels_non_bgcolor );
    } // end while
    adjustment_parameter -= PARAMETER_INTERVAL;

    return adjustment_parameter;
} // End calcAdjustmentParameter()

float BrightnessAdjustment::calcTemporaryPercent( const kvs::ColorImage& color_image, const float p_current, const kvs::UInt8 threshold_pixel_value_LR1, const size_t npixels_non_bgcolor )
{
    kvs::ColorImage color_image_tmp = deepCopyColorImage( color_image );
    execBrightnessAdjustment( color_image_tmp, p_current );

    // Convert color to gray
    const kvs::GrayImage gray_image_tmp( color_image_tmp );

    int counter = 0;
    for ( size_t j = 0; j < gray_image_tmp.height(); j++ )
        for ( size_t i = 0; i < gray_image_tmp.width(); i++ )
            if ( gray_image_tmp.pixel( i, j ) >= threshold_pixel_value_LR1 ) 
                counter++;

    const float percent_tmp = float( counter ) / float( npixels_non_bgcolor );
    return percent_tmp;
} // End calcTemporaryPercent()

kvs::ColorImage BrightnessAdjustment::deepCopyColorImage( const kvs::ColorImage& other ) const
{
    kvs::ColorImage duplicated_color_image( other.width(), other.height() );

    for ( size_t j = 0; j < other.height(); j++ )
        for ( size_t i = 0; i < other.width(); i++ )
            duplicated_color_image.setPixel( i, j, other.pixel( i, j ) );

    return duplicated_color_image;
} // End deepCopyColorImage()

inline float BrightnessAdjustment::specifyNumberOfDigits( const float p, const float digits ) const
{
    std::stringstream ss;
    ss << p;
    std::string p_str = ss.str();

    return stof( p_str.substr( 0, digits ) );
} // End specifyNumberOfDigits()

void BrightnessAdjustment::execBrightnessAdjustment( kvs::ColorImage& color_image, const float p ) const
{
    kvs::RGBColor pixel;
    kvs::UInt8    r, g, b;

    for ( size_t j = 0; j < color_image.height(); j++ ) {
        for ( size_t i = 0; i < color_image.width(); i++ ) {
            pixel = color_image.pixel( i, j );
            
            // Check whether the pixel value exceeds 255 after adjusting.
            if ( pixel.r() * p > 255 ) r = 255;
            else                       r = pixel.r() * p;
            
            if ( pixel.g() * p > 255 ) g = 255;
            else                       g = pixel.g() * p;

            if ( pixel.b() * p > 255 ) b = 255;
            else                       b = pixel.b() * p;

            // Apply adjustment
            pixel.set( r, g, b );
            color_image.setPixel( i, j, pixel );
        } // end for
    } // end for
} // End execBrightnessAdjustment()

float BrightnessAdjustment::calcFinalPercent( const kvs::ColorImage& color_image, const kvs::UInt8 threshold_pixel_value_LR1, const size_t npixels_non_bgcolor ) const
{
    // Convert color to gray
    const kvs::GrayImage gray_image_tmp( color_image );

    int counter = 0;
    for ( size_t j = 0; j < gray_image_tmp.height(); j++ )
        for ( size_t i = 0; i < gray_image_tmp.width(); i++ )
            if ( gray_image_tmp.pixel( i, j ) >= threshold_pixel_value_LR1 ) 
                counter++;

    const float percent_final = float( counter ) / float( npixels_non_bgcolor );
    return percent_final;
} // End calcFinalPercent()

inline void BrightnessAdjustment::writeAdjustedImage( const std::string filename , const kvs::ColorImage& color_image, const float p_final ) const
{
    std::ostringstream oss;
    oss << p_final*100;
    std::string adjusted_image_filename( filename + "_adjusted" + oss.str() + ".bmp" );
    color_image.write( adjusted_image_filename );

    std::cout << "\n*** The adjusted image is saved as a BITMAP file."   << std::endl;
    std::cout << "    PATH: " << adjusted_image_filename << std::endl;
    std::cout << "===========================================\n" << std::endl;

    // Exec. open command (macOS only)
#ifdef OS_MAC
    execOpenCommand( adjusted_image_filename );
#endif
} // End writeAdjustedImage()

inline void BrightnessAdjustment::execOpenCommand( const std::string filename ) const
{
    std::string EXEC( "open " );
    EXEC += filename;
    system( EXEC.c_str() );
} // End execOpenCommand()

void BrightnessAdjustment::AdjustBrightnessDivideVersion( const std::string filename )
{
    // Display opening message
    displayMessage();

    // Calc number of pixels of the image
    const size_t npixels             = m_color_image.numberOfPixels();
    const size_t npixels_non_bgcolor = calcNumberOfPixelsNonBGColor( m_color_image );
    std::cout   << "*** Number of pixels                  : " 
                << npixels             << " (pixels)" << std::endl;
    std::cout   << "*** Number of pixels non-BGColor      : " 
                << npixels_non_bgcolor << " (pixels)" << std::endl;

    // Convert color to gray
    const kvs::GrayImage gray_image( m_color_image );
    const kvs::GrayImage gray_image_LR1( m_color_image_LR1 );

    // =====================================================
    //  STEP1: Calculate threshold pixel value for division
    // =====================================================
    const kvs::UInt8 th4division = discriminantAnalysis( gray_image );
    std::cout   << "*** Threshold to divide the image     : " 
                << +th4division << " (pixel value)" << std::endl;

    // ===================================================
    //  STEP2: Divide into high and low pixel value image
    // ===================================================
    // Initialize
    m_color_image_high = deepCopyColorImage( m_color_image );
    m_color_image_low  = deepCopyColorImage( m_color_image );

    // Divide into high and low pixel value image
    divideIntoTwoImages( gray_image, th4division );
    
    // Save two images
    m_color_image_high.write( filename + "_high_before.bmp" );
    m_color_image_low.write( filename + "_low_before.bmp" );

    const clock_t start = clock();
    std::cout   << "\n*** Executing \"Brightness Adjustment\"..." << std::endl;
    // ========================================================
    //  STEP3: Adjust brightness of the high-pixel-value image
    // ========================================================
    const kvs::UInt8 max_pixel_value     = calcMaxPixelValue( gray_image );
    const kvs::UInt8 max_pixel_value_LR1 = calcMaxPixelValue( gray_image_LR1 );
    std::cout   << "*** Max pixel value                   : " 
                << +max_pixel_value << " (pixel value)"     << std::endl;
    std::cout   << "*** Max pixel value (LR=1)            : " 
                << +max_pixel_value_LR1 << " (pixel value)" << std::endl;

    const size_t npixels_non_bgcolor_LR1 = calcNumberOfPixelsNonBGColor( m_color_image_LR1 );
    const kvs::UInt8 threshold_pixel_value_LR1_high = searchThresholdPixelValue( gray_image_LR1, npixels_non_bgcolor_LR1, max_pixel_value_LR1 );

    float p_high = calcAdjustmentParameter( m_color_image_high, threshold_pixel_value_LR1_high, npixels_non_bgcolor );
    p_high = specifyNumberOfDigits( p_high, 4 );
    execBrightnessAdjustment( m_color_image_high, p_high );

    m_color_image_high.write( filename + "_high_after.bmp" );

    // =======================================================
    //  STEP4: Adjust brightness of the low-pixel-value image
    // =======================================================
    const kvs::UInt8 target_pixel_value = calcMeanPixelValue( m_color_image_high );
    const kvs::UInt8 threshold_pixel_value_LR1_low = searchThresholdPixelValue( gray_image_LR1, npixels_non_bgcolor_LR1, target_pixel_value );

    float p_low = calcAdjustmentParameter( m_color_image_low, threshold_pixel_value_LR1_low, npixels_non_bgcolor );
    p_low = specifyNumberOfDigits( p_low, 4 );
    execBrightnessAdjustment( m_color_image_low, p_low );

    m_color_image_low.write( filename + "_low_after.bmp" );

    // =============================================================
    //  STEP5: Combine the adjusted high and low pixel value images
    // =============================================================
    combineTwoImages( m_color_image_high, m_color_image_low );


    const clock_t end = clock();
    std::cout   << "*** Done! ( " 
                << static_cast<double>( end - start ) / CLOCKS_PER_SEC << " [sec] )\n" << std::endl;

    std::cout   << "*** Adjustment parameter              : "
                << "p_high: " << std::setprecision(3) << p_high << ", "
                << "p_low: "  << std::setprecision(3) << p_low  << std::endl;

    m_color_image.write( filename + "_final.bmp" );

} // End AdjustBrightness4Divide()

kvs::UInt8 BrightnessAdjustment::discriminantAnalysis( const kvs::GrayImage& gray_image ) const
{
    // Create histogram
    std::vector<kvs::UInt8> hist( 256, 0 );
    for ( size_t j = 0; j < gray_image.height(); j++ )
        for ( size_t i = 0; i < gray_image.width(); i++ )
            hist[ gray_image.pixel( i, j ) ]++;
    // end for
    
    // Discriminant Analysis
    kvs::UInt8 threshold = 0;
    double max = 0.0;  // max value of {w1 * w2 * (m1 - m2)^2}
    for ( size_t i = 0; i < 256; i++ ) {
        int w1 = 0;  
        int w2 = 0;
        long sum1 = 0;
        long sum2 = 0;
        double m1 = 0.0;
        double m2 = 0.0;
        
        for ( int j = 0; j < i; j++ ) {
            w1   += hist[j];
            sum1 += j * hist[j];
        }
        
        for ( int j = i + 1; j < 256; j++ ) {
            w2   += hist[j];
            sum2 += j * hist[j];
        }
        
        if ( w1 ) m1 = (double)sum1 / w1;
        if ( w2 ) m2 = (double)sum2 / w2;
        
        const double tmp = ( (double)w1 * w2 * (m1 - m2) * (m1 - m2) );
        if ( tmp > max ) {
            max = tmp;
            threshold = i;
        }
    } // end for
    
    return threshold;
} // End discriminantAnalysis()

void BrightnessAdjustment::divideIntoTwoImages( const kvs::GrayImage& gray_image, const kvs::UInt8 threshold ) {
    const kvs::RGBColor BGColor( 0, 0, 0 );

    for ( size_t j = 0; j < gray_image.height(); j++ ) {
        for ( size_t i = 0; i < gray_image.width(); i++ ) {
            // Divide
            if ( gray_image.pixel( i, j ) > threshold )
                m_color_image_low.setPixel( i, j, BGColor );
            else
                m_color_image_high.setPixel( i, j, BGColor );
            // end if
        } // end for i
    } // end for j

} // End divideIntoTwoImages()

kvs::UInt8 BrightnessAdjustment::calcMeanPixelValue( const kvs::ColorImage& color_image ) const
{
    // Convert color to gray
    const kvs::GrayImage gray_image( color_image );
    
    int sum_pixel_value = 0;
    int npixels_non_bgcolor = 0;
    for ( size_t j = 0; j < gray_image.height(); j++ ) {
        for ( size_t i = 0; i < gray_image.width(); i++ ) {
            if ( gray_image.pixel( i, j ) != 0 ) {
                sum_pixel_value += (int)gray_image.pixel( i, j );
                npixels_non_bgcolor++;
            }
        }
    }
    
    const kvs::UInt8 mean_pixel_value = sum_pixel_value / npixels_non_bgcolor;
    return mean_pixel_value;
} // End calcMeanPixelValue()

void BrightnessAdjustment::combineTwoImages( const kvs::ColorImage& color_image_high, const kvs::ColorImage& color_image_low )
{
    for ( size_t j = 0; j < color_image_high.height(); j++ ) {
        for ( size_t i = 0; i < color_image_high.width(); i++ ) {
            if ( color_image_high.pixel( i, j ) == m_bgcolor ) {
            } else { m_color_image.setPixel( i, j, color_image_high.pixel( i, j ) ); }

            if ( color_image_low.pixel( i, j ) == m_bgcolor ) {
            } else { m_color_image.setPixel( i, j, color_image_low.pixel( i, j ) ); }
        }
    }
} // End combineTwoImages()