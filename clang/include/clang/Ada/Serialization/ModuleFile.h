//===- ModuleFile.h - Binary Module File Format -----------------*- C++ -*-===//
//
// Part of the Ada 2012 Frontend for Clang.
//
// Binary module file format for serialized Ada compilation units.
// Format:
//   Header (64 bytes):
//     Magic: "ADAMOD1\0" (8 bytes)
//     Version: u32 (1 for initial)
//     Source hash: u64 (MurmurHash3 of source)
//     Source timestamp: i64
//     Unit name length: u32
//     AST text length: u64
//     Padding to 64 bytes
//   Unit name: variable length string (null-terminated)
//   AST text: variable length text (serialized via ast-dump format)
//
//===----------------------------------------------------------------------===//

#ifndef CLANG_ADA_SERIALIZATION_MODULEFILE_H
#define CLANG_ADA_SERIALIZATION_MODULEFILE_H

#include "clang/Ada/AST/AST.h"
#include "clang/Ada/Parser/AdaParser.h"
#include <cstdint>
#include <string>
#include <vector>

namespace clang::ada::serialization {

constexpr uint8_t MODULE_MAGIC[8] = {'A', 'D', 'A', 'M', 'O', 'D', '1', '\0'};
constexpr uint32_t MODULE_VERSION = 1;
constexpr size_t MODULE_HEADER_SIZE = 64;

// Compute a hash of the source text (MurmurHash3 variant)
uint64_t hashSource(const std::string &Source);

// Write a module file from an AST
bool writeModuleFile(const std::string &Path,
                     const std::string &UnitName,
                     const std::string &Source,
                     int64_t Timestamp,
                     const LibraryUnit *AST);

// Read a module file and return the AST
std::unique_ptr<LibraryUnit> readModuleFile(const std::string &Path);

// Check if a module file exists and is up-to-date
bool moduleFileIsCurrent(const std::string &ModulePath,
                         const std::string &SourcePath);

// Get the module file path for a given spec
std::string getModuleFilePath(const std::string &SpecPath);

} // namespace clang::ada::serialization

#endif // CLANG_ADA_SERIALIZATION_MODULEFILE_H
