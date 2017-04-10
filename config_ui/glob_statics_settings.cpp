//Oleksiy Zakharov, 2016, alexzkhr@gmail.com

#include "globalsettings.h"

#define DECL_SETT(CLASS,KEY,DEF,args...) {KEY, widgetted_pt(new CLASS(KEY,DEF,args))}
//-----------------------------------------------------------------------------------------------------------------------
//declare all global settings below, so it can be automatically added to setting's
//-----------------------------------------------------------------------------------------------------------------------

const StaticSettingsMap &StaticSettingsMap::getGlobalSetts()
{
    //do not change keys once used, because 1: key-string is  directly used in other code (must be same), 2: users will lose stored value on next run
    //visual order depends on string sort of the keys
    const static StaticSettingsMap list({
#ifdef USING_VIDEO_FS
                                            DECL_SETT(GlobalStorableBool, "Bool_cache_frames", false, tr("Cache extracted Video Frames."),
                                            tr("If enabled will store all extracted video frames in \".cached_frames\" subfolder.\n"
                                            "This will consume significant HDD space (much-much more then original video).\n"
                                            "It is really slow on HDD (not SSD)."
                                            )),

                                            DECL_SETT(GlobalStorableBool, "Bool_parse_frames", true, tr("Parse Videos into Frames"),
                                            tr("If enabled will split videos into frames, this may take significant time.")),
#endif
                                            //fixme: not too clean with this, loading project will trigger switchFolder, which will trigger loading etc...
//                                            DECL_SETT(GlobalStorableBool, "Bool_autoloadproj", true, tr("If folder has single project file - load it."),
//                                            tr("If enabled it will automatically load projects when folder is switched (only if there is 1 exactly project file found).")),
                                            DECL_SETT(GlobalStorableInt, "Int_scroll_delay", 50, tr("Scroll delay (ms)"), tr("This value defines delay-on-scroll before actual previews will be loaded.\n"
                                            "You can set value depend on how fast is your storage to pick best reaction/device pressure ratio."), 25, 500),
                                        });
    return list;
}
