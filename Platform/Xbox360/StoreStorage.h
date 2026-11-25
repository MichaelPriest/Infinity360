#pragma once
#include "FileWorkspace.h"
#include "SavedFile.h"
namespace StoreStorage {
inline bool Valid(const std::string& id){return id.empty()||id=="hdd"||id=="usb0"||id=="usb1"||id=="usb2";}
inline std::string Personal(const std::string& root,const std::string& id){
    if(id.empty())return FileWorkspace::Join(root,"UserFiles");
#ifdef __XSDK__
    if(Valid(id))return id+":/Infinity360Files";
#endif
    return "";
}
inline std::string Preference(const std::string& root){return FileWorkspace::Join(root,"infinity360-store-storage.txt");}
inline std::string Read(const std::string& root){
    const auto path=savedFile(Preference(root));if(!fileExists(path))return "";
    std::ifstream in(path.c_str(),std::ios::binary);std::string id,extra;
    if(!std::getline(in,id)||std::getline(in,extra)||!Valid(id))return "invalid";
    return id;
}
inline bool Save(const FileWorkspace& ws,const std::string& root,const std::string& id){
    const auto folder=Personal(root,id);
    if(!Valid(id)||folder.empty()||!ws.CanWrite(folder)||!ws.CanWrite(Personal(root,"")))return false;
    const auto path=Preference(root);std::ofstream out((path+".tmp").c_str(),std::ios::binary|std::ios::trunc);
    out<<id<<"\n";out.close();return out&&replaceSavedFile(path);
}
inline std::string Queue(const std::string& root,const std::string& id){
    if(!Valid(id))return "";
    return FileWorkspace::Join(root,id.empty()?"infinity360-native-queue.tsv":"infinity360-native-"+id+".tsv");
}
inline std::string Label(const std::string& id){
    return id.empty()?"Local (UserFiles)":id=="hdd"?"HDD":id=="usb0"?"USB 0":id=="usb1"?"USB 1":id=="usb2"?"USB 2":"Configuracao invalida";
}
}
