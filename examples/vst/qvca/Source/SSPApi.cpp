
#include <Percussa.h>

#include "PluginProcessor.h"
#include "PluginEditor.h"

// this is a common file in ALL my plugin projects
// it is copied between them, so do not alter it without propagating to others.

// there are two reasons for it to be duplicated
// - it has to reference the specific PluginProcessor
// - there are static functions , and we may want different variations

// note: if a plugin needs a variation , the COPY the file and rename it
// and ref new file name in cmake, this way f I propagate a changed version it will NOT
// overwrite the custom version !
// this is HIGHLY likely to occur !!!!

enum SSPButtons {
    SSP_Soft_1,
    SSP_Soft_2,
    SSP_Soft_3,
    SSP_Soft_4,
    SSP_Soft_5,
    SSP_Soft_6,
    SSP_Soft_7,
    SSP_Soft_8,
    SSP_Left,
    SSP_Right,
    SSP_Up,
    SSP_Down,
    SSP_Shift_L,
    SSP_Shift_R,
    SSP_LastBtn
};

//SSPHASH
#define SSP_IMAGECACHE_HASHCODE 0x53535048415348


class SSP_PluginEditorInterface : public Percussa::SSP::PluginEditorInterface {
public:
    SSP_PluginEditorInterface(PluginEditor *editor) :
        editor_(editor) {
    }

    ~SSP_PluginEditorInterface() override {
        if (editor_) delete editor_;
    }

    void frameStart() override {
        PluginEditorInterface::frameStart();
    }

    void visibilityChanged(bool b) override {
        PluginEditorInterface::visibilityChanged(b);
    }

    void renderToImage(unsigned char *buffer, int width, int height) override {
        Image img = ImageCache::getFromHashCode(SSP_IMAGECACHE_HASHCODE);
        if (!img.isValid()) {
            // this will cache image for use on all instances of this plugin
            Image newimg(Image::ARGB, width, height, true);
            ImageCache::addImageToCache(newimg, SSP_IMAGECACHE_HASHCODE);
            img = newimg;
        }

        if (!editor_->isVisible()) {
            editor_->setBounds(Rectangle<int>(0, 0, width, height));
            editor_->setOpaque(true);
            editor_->setVisible(true);
        }

        // draw editor component onto image.
        Graphics g(img);
        editor_->paintEntireComponent(g, true);
        Image::BitmapData bitmap(img, Image::BitmapData::readOnly);
        memcpy(buffer, bitmap.data, width * height * 4);
    }

    void buttonPressed(int n, bool val) {
        if (n <= SSP_Soft_8) {
            editor_->onButton(n, val);
        } else {
            switch (n) {
                case SSP_Left : {
                    editor_->onLeftButton(val);
                    break;
                }
                case SSP_Right : {
                    editor_->onRightButton(val);
                    break;
                }
                case SSP_Up : {
                    editor_->onUpButton(val);
                    break;
                }
                case SSP_Down : {
                    editor_->onDownButton(val);
                    break;
                }
                case SSP_Shift_L : {
                    editor_->onLeftShiftButton(val);
                    break;
                }
                case SSP_Shift_R : {
                    editor_->onRightShiftButton(val);
                    break;
                }
                default : {
                    // ignore
                }
            }
        }
    }

    void encoderPressed(int n, bool val) {
        editor_->onEncoderSwitch(n, val);
    }

    void encoderTurned(int n, int val) {
        editor_->onEncoder(n, (float) val);
    }

private:
    PluginEditor *editor_;
};



class SSP_PluginInterface : public Percussa::SSP::PluginInterface {
public:
    SSP_PluginInterface(PluginProcessor *p) : processor_(p), editor_(nullptr) {
    }

    ~SSP_PluginInterface() {
        if(editor_) delete editor_;
        if(processor_) delete processor_;
    }


    Percussa::SSP::PluginEditorInterface *getEditor() override {
        if (editor_ == nullptr) {
            PluginEditor *pluginEditor = static_cast<PluginEditor *>( processor_->createEditor());
            editor_ = new SSP_PluginEditorInterface(pluginEditor);
        }
        return editor_;
    }

    void buttonPressed(int n, bool val) override {
        if (editor_) editor_->buttonPressed(n, val);
    }

    void encoderPressed(int n, bool val) override {
        if (editor_) editor_->encoderPressed(n, val);
    }

    void encoderTurned(int n, int val) override {
        if (editor_) editor_->encoderTurned(n, val);
    }

    void inputEnabled(int n, bool val) override {
        processor_->onInputChanged(n, val);
    }

    void outputEnabled(int n, bool val) override {
        processor_->onOutputChanged(n, val);
    }

    void getState(void **buffer, size_t *size) override {
        MemoryBlock state;
        processor_->getStateInformation(state);
        *size = state.getSize();
        *buffer = new char[*size];
        state.copyTo(*buffer, 0, *size);
    }

    void setState(void *buffer, size_t size) override {
        processor_->setStateInformation(buffer, size);
    }

    void prepare(double sampleRate, int samplesPerBlock) override {
        unsigned numIn = processor_->getBusCount(true);
        unsigned numOut = processor_->getBusCount(false);
        processor_->setPlayConfigDetails(
            numIn,
            numOut,
            sampleRate,
            samplesPerBlock);

        processor_->prepareToPlay(sampleRate, samplesPerBlock);
    }

    void process(float **channelData, int numChannels, int numSamples) override {
        MidiBuffer midiBuffer;
        AudioSampleBuffer buffer(channelData, numChannels, numSamples);
        processor_->processBlock(buffer, midiBuffer);
    }

private:
    SSP_PluginEditorInterface *editor_ = nullptr;
    PluginProcessor *processor_ = nullptr;
};


extern "C" __attribute__ ((visibility("default")))
Percussa::SSP::PluginDescriptor *createDescriptor() {
    std::vector<std::string> inNames;
    std::vector<std::string> outNames;
    auto busProps = PluginProcessor::getBusesProperties();

    for (auto layout: busProps.inputLayouts) {
        inNames.push_back(layout.busName.toStdString());
    }
    for (auto layout: busProps.outputLayouts) {
        outNames.push_back(layout.busName.toStdString());
    }

    auto desc = new Percussa::SSP::PluginDescriptor;

    desc->name = JucePlugin_Name;
    desc->descriptiveName = JucePlugin_Desc;
    desc->manufacturerName = JucePlugin_Manufacturer;
    desc->version = JucePlugin_VersionString;
    desc->uid = (int) JucePlugin_VSTUniqueID;
    desc->inputChannelNames = inNames;
    desc->outputChannelNames = outNames;

    return desc;
}


extern "C" __attribute__ ((visibility("default")))
Percussa::SSP::PluginInterface *createInstance() {
    return new SSP_PluginInterface(new PluginProcessor());
}


extern "C" __attribute__ ((visibility("default")))
void getApiVersion(unsigned &major, unsigned &minor) {
    major = Percussa::SSP::API_MAJOR_VERSION;
    minor = Percussa::SSP::API_MINOR_VERSION;
}


