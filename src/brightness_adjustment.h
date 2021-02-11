////////////////////////////////////////
//   @file   brightness_adjustment.h
//   @author Tomomasa Uchida
//   @date   2020/09/30
////////////////////////////////////////

#if !defined  BRIGHTNESS_ADJUSTMENT_H
#define       BRIGHTNESS_ADJUSTMENT_H

class BrightnessAdjustment {
public:
    enum FILE_FORMAT4BA
    {
        SPBR_ASCII4BA  = 0,
        SPBR_BINARY4BA = 1,
        PLY_ASCII4BA   = 2,
        PLY_BINARY4BA  = 3,
    };

    enum ADJUSTMENT_TYPE
    {
        OFF     = 0,
        UNIFORM = 1,
        DIVIDE  = 2
    };

    // Constructor
    BrightnessAdjustment( const FILE_FORMAT4BA file_format, const int id );

    // Functions to control object and renderer
public:
    void   RegisterObject( kvs::Scene* scene, int argc, char** argv, SPBR* spbr_engine, const size_t repeat_level );
    void   ReplaceObject( kvs::Scene* scene, int argc, char** argv, SPBR* spbr_engine, const size_t repeat_level );
    void   SnapshotImage( kvs::Scene* scene, const std::string filename, const int repeat_level );
    size_t getSnapshotCounter() const { return m_snapshot_counter; };
private:
    kvs::PointObject* CreateObject( int argc, char** argv );
    kvs::PointObject* CreateObjectCommon( int argc, char** argv, SPBR* spbr_engine );
    kvs::glsl::ParticleBasedRenderer* CreateRenderer( SPBR* spbr_engine, const size_t repeat_level );


    // Common functions to adjust the brightness of the image
public:
    int         getAdjustmentType() const { return m_adjustment_type; };
private:
    void        displayMessage() const;
    kvs::UInt8  searchThresholdPixelValue4LR1( const kvs::GrayImage& gray_image, const size_t npixels_non_bgcolor, const kvs::UInt8 right_edge_pixel_value, const float percent_in_ref_sec ) const;
    int         calcNumberOfPixelsNonBGColor( const kvs::ColorImage& image ) const;
    kvs::UInt8  calcMaxPixelValue( const kvs::GrayImage& image ) const;
    float       calcAdjustmentParameter( const kvs::ColorImage& color_image, const kvs::UInt8 threshold_pixel_value_LR1, const size_t npixels_non_bgcolor, const float percent_in_ref_sec );
    float       calcTemporaryPercent( const kvs::ColorImage& color_image, const float p_current, const kvs::UInt8 threshold_pixel_value_LR1, const size_t npixels_non_bgcolor );
    kvs::ColorImage deepCopyColorImage( const kvs::ColorImage& other ) const;
    float       specifyNumberOfDigits( const float p, const float digits ) const;
    void        execBrightnessAdjustment( kvs::ColorImage& color_image, const float p ) const;
    void        execOpenCommand( const std::string filename ) const;

    //---------- DATA ----------//
    const FILE_FORMAT4BA    m_file_format;
    ADJUSTMENT_TYPE         m_adjustment_type;
    const kvs::RGBColor     m_bgcolor;
    size_t                  m_snapshot_counter;
    kvs::ColorImage         m_color_image;
    kvs::ColorImage         m_color_image_LR1;


    // Functions to support BA: Uniform Version
public:
    void        AdjustBrightnessUniformVersion( const std::string filename );
private:
    float       calcFinalPercent( const kvs::ColorImage& color_image, const kvs::UInt8 threshold_pixel_value_LR1, const size_t npixels_non_bgcolor ) const;
    void        writeAdjustedImage( const std::string filename, const float p_final ) const;


    // Functions to support BA: Divide Version
public:
    void        AdjustBrightnessDivideVersion( const std::string filename );
private:
    kvs::UInt8  discriminantAnalysis( const kvs::GrayImage& gray_image ) const;
    void        divideIntoTwoImages( const kvs::GrayImage& gray_image, const kvs::UInt8 threshold );
    kvs::UInt8  calcMeanPixelValue( const kvs::ColorImage& color_image ) const;
    void        combineTwoImages( const kvs::ColorImage& color_image_high, const kvs::ColorImage& color_image_low );
    void        writeAdjustedImage( const std::string filename, const float p_high, const float p_low ) const;

    //---------- DATA ----------//
    kvs::ColorImage         m_color_image_high;
    kvs::ColorImage         m_color_image_low;
};

#endif // end of brightness_adjustment.h