/*
 * funkinchrpak by UNSTOP4BLE (not the one ckdev made)
 * Packs characters into a binary file for the PSX port
*/

#include <iostream>
#include <fstream>

#include "json.hpp"
using json = nlohmann::json;

typedef int32_t fixed_t;
#define FIXED_SHIFT (10)
#define FIXED_UNIT  (1 << FIXED_SHIFT)
#define FIXED_DEC(d, f) ((fixed_t)(((int64_t)(d) * FIXED_UNIT) / (f)))

#define ASCR_REPEAT 0xFF
#define ASCR_CHGANI 0xFE
#define ASCR_BACK   0xFD

#define CHAR_SPEC_MISSANIM (1 << 0) 

struct Animation
{
    //Animation data and script
    uint8_t spd;
    uint8_t script[255];
};

struct CharFrame
{
    uint8_t tex;
    uint16_t src[4];
    int16_t off[2];
};

std::vector<std::string> charAnim{
    "CharAnim_Idle",
    "CharAnim_Left",  "CharAnim_LeftAlt",
    "CharAnim_Down",  "CharAnim_DownAlt",
    "CharAnim_Up",    "CharAnim_UpAlt",
    "CharAnim_Right", "CharAnim_RightAlt",
};

std::vector<std::string> playerAnim{
    "PlayerAnim_LeftMiss",
    "PlayerAnim_DownMiss",
    "PlayerAnim_UpMiss",
    "PlayerAnim_RightMiss",
    
    "PlayerAnim_Peace",
    "PlayerAnim_Sweat",

    "PlayerAnim_Dead0", 
    "PlayerAnim_Dead1", 
    "PlayerAnim_Dead2", 
    "PlayerAnim_Dead3", 
    "PlayerAnim_Dead4",
    "PlayerAnim_Dead5", 
    
    "PlayerAnim_Dead6", 
    "PlayerAnim_Dead7", 
};

struct CharacterFileHeader
{
    int32_t size_struct;
    int32_t size_frames;
    int32_t size_animation;
    int32_t sizes_scripts[32]; // size of charAnim vector + playeranim vector
    int32_t size_textures;

    //Character information
    uint16_t spec;
    uint16_t health_i; //hud1.tim
    uint32_t health_bar; //hud1.tim
    char archive_path[128];
    fixed_t focus_x, focus_y, focus_zoom;
};

std::vector<std::string> charStruct;

int getEnumFromString(std::vector<std::string> &compare, std::string str) {
    for (int i = 0; i < compare.size(); i++)
        if (strcmp(compare[i].c_str(), str.c_str()) == 0) return i;

    std::cout << "invalid enum " << str << std::endl;   
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << "usage: funkinchrpak out_chr in_json" << std::endl;
        return 1;
    }
    
    //Read json
    std::ifstream file(argv[2]);
    if (!file.is_open())
    {
        std::cout << "Failed to open " << argv[2] << std::endl;
        return 1;
    }
    json j;
    file >> j;

    CharacterFileHeader new_char;

    bool isstr = j["spec"].is_string();
    if (isstr)
    {
        std::string spec = j["spec"];
        if (spec == "CHAR_SPEC_MISSANIM")
            new_char.spec = CHAR_SPEC_MISSANIM;

    }
    else new_char.spec = 0;

    new_char.health_i = j["health_i"];
    std::string health_bar_str = j["health_bar"];
    new_char.health_bar = std::stoul(health_bar_str, nullptr, 16);
    std::string arcpath = j["archive"];
    strncpy(new_char.archive_path, arcpath.c_str(), sizeof(new_char.archive_path));
    new_char.focus_x = FIXED_DEC(j["focus"][0][0], (int)j["focus"][0][1]);
    new_char.focus_y = FIXED_DEC(j["focus"][1][0], (int)j["focus"][1][1]);
    new_char.focus_zoom = FIXED_DEC(j["focus"][2][0], (int)j["focus"][2][1]);

    //parse animation
    for (int i = 0; i < j["struct"].size(); i++) 
        charStruct.push_back(j["struct"][i]);

    new_char.size_struct = j["struct"].size();
    CharFrame frames[j["frames"].size()];
    new_char.size_frames = j["frames"].size();
    //parse frames
    for (int i = 0; i < j["frames"].size(); i++) {
        std::string texstr = j["frames"][i][0];
        frames[i].tex = getEnumFromString(charStruct, j["frames"][i][0]);
        for (int i2 = 0; i2 < 4; i2++)
            frames[i].src[i2] = j["frames"][i][1][i2];
        frames[i].off[0] = j["frames"][i][2][0];
        frames[i].off[1] = j["frames"][i][2][1];
    }

    Animation anims[j["animation"].size()];     
    std::vector<std::vector<uint16_t>> scripts;
    new_char.size_animation = j["animation"].size();

    for (int i = 0; i < j["animation"].size(); i++) {
        scripts.resize(j["animation"].size());
        scripts[i].resize(j["animation"][i][1].size());
        new_char.sizes_scripts[i] = j["animation"][i][1].size();

        anims[i].spd = j["animation"][i][0];

        for (int i2 = 0; i2 < j["animation"][i][1].size(); i2++) {
            if (i2 < j["animation"][i][1].size()-2)
            {
             //   std::cout << j["animation"][i][1][i2] << std::endl;
                scripts[i][i2] = j["animation"][i][1][i2];
            }
            else //change string to number
            {
                if (i2 == j["animation"][i][1].size()-2) { // ascr mode
             //       std::cout << j["animation"][i][1][i2] << std::endl;
                    bool isrepeat = j["animation"][i][1][i2].is_string();
                    std::string ascrmode;
                    if (!isrepeat && j["animation"][i][1][i2+1] == "ASCR_REPEAT")
                        ascrmode = j["animation"][i][1][i2+1];
                    else
                        ascrmode = j["animation"][i][1][i2];

                    if (ascrmode == "ASCR_REPEAT") {
                        scripts[i][i2] = ASCR_REPEAT;
                        break;
                    }
                    else if (ascrmode == "ASCR_BACK")
                        scripts[i][i2] = ASCR_BACK;
                    else if (ascrmode == "ASCR_CHGANI")
                        scripts[i][i2] = ASCR_CHGANI;
                }
                if (i2 == j["animation"][i][1].size()-1) // back animation
                {
                    bool isstring = j["animation"][i][1][i2].is_string();
                    if (isstring) {
                        std::string backanim = j["animation"][i][1][i2];
                        if (i > charAnim.size())
                            scripts[i][i2] = charAnim.size()+getEnumFromString(playerAnim, backanim);
                        else
                            scripts[i][i2] = getEnumFromString(charAnim, backanim);
                    }
                    else
                        scripts[i][i2] = j["animation"][i][1][i2];
                    //std::cout << j["animation"][i][1][i2] << std::endl;
                }
            }
        }
    }
    

    //copy over the shit into the arrays 
    Animation animations[new_char.size_animation];
    for (int i = 0; i < new_char.size_animation; i++)
    {
        animations[i].spd = anims[i].spd;
        for (int i2 = 0; i2 < 256; i2++)
            animations[i].script[i2] = scripts[i][i2];
    }

    //textures
    new_char.size_textures = j["textures"].size();
    
    char texpaths[new_char.size_textures][32];
    for (int i = 0; i < j["textures"].size(); i++) {
        std::string curtex = j["textures"][i];
        strncpy(texpaths[i], curtex.c_str(), 32);
    }

    std::ofstream binFile(std::string(argv[1]), std::ostream::binary);
    binFile.write(reinterpret_cast<const char*>(&new_char), sizeof(new_char));
    binFile.write(reinterpret_cast<const char*>(&animations), sizeof(animations));
    binFile.write(reinterpret_cast<const char*>(&frames), sizeof(frames));
    binFile.write(reinterpret_cast<const char*>(&texpaths), sizeof(texpaths));
    binFile.close();  

    std::cout << "success" << std::endl;
/*
    //test reading
    CharacterFileHeader testchar; 
    std::ifstream inFile(argv[1], std::istream::binary); 
    inFile.read(reinterpret_cast<char *>(&testchar), sizeof(testchar)); 
    Animation animationstest[testchar.size_animation]; 
    CharFrame framestest [testchar.size_frames]; 
    char textest[testchar.size_textures][32]; 
    inFile.read(reinterpret_cast<char *>(&animationstest), testchar.size_animation * sizeof(Animation)); 
    inFile.read(reinterpret_cast<char *>(&framestest), testchar.size_frames * sizeof(CharFrame)); 
    inFile.read(reinterpret_cast<char *>(&textest), testchar.size_textures * 32); 
    inFile.close();   

    //print header
    std::cout << "spec " << static_cast<unsigned int>(testchar.spec) << std::endl;
    std::cout << "icon " << static_cast<unsigned int>(testchar.health_i) << std::endl;
    std::cout << "healthbar " << static_cast<unsigned int>(testchar.health_bar) << std::endl;
    std::cout << "cx " << testchar.focus_x << std::endl;
    std::cout << "cy " << testchar.focus_y << std::endl;
    std::cout << "cz " << testchar.focus_zoom << std::endl;

    //print frames array
    for (int i = 0; i < testchar.size_frames; ++i)
    {
        std::cout << "tex " << static_cast<unsigned int>(framestest[i].tex) << " frames " << framestest[i].src[0] << " " << framestest[i].src[1]  << " " << framestest[i].src[2]  <<" " << framestest[i].src[3] <<" offset " << framestest[i].off[0]  << " " << framestest[i].off[1]  <<" " <<  std::endl;
    }   

    //print script arrays
    for (int i = 0; i < testchar.size_animation; i++) {
        std::cout << "speed " << static_cast<unsigned int>(animationstest[i].spd) << " frames";
        for (int i2 = 0; i2 < testchar.sizes_scripts[i]; i2 ++)
            std::cout << " " << static_cast<unsigned int>(animationstest[i].script[i2]);
        std::cout << std::endl;
    }

    //print texture array        
    std::cout << "tex" << std::endl;
    for (int i = 0; i < testchar.size_textures; ++i)
    {
        for (int i2 = 0; i2 < 32; i2++)
            std::cout << textest[i][i2];
        std::cout << std::endl;
    }   */

    return 0;
}