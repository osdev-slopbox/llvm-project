//===- ModuleFile.cpp - Binary Module File Format -----------------*- C++ -*-===//
//
// Part of the Ada 2012 Frontend for Clang.
//
//===----------------------------------------------------------------------===//

#include "clang/Ada/Serialization/ModuleFile.h"
#include "clang/Ada/Parser/AdaParser.h"
#include <cstdio>
#include <cstring>
#include <sstream>
#include <fstream>
#include <iostream>
#include <sys/stat.h>

namespace clang::ada::serialization {

// Simple FNV-1a hash
uint64_t hashSource(const std::string &Source) {
  uint64_t hash = 14695981039346656037ULL;
  for (unsigned char c : Source) {
    hash ^= c;
    hash *= 1099511628211ULL;
  }
  return hash;
}

std::string getModuleFilePath(const std::string &SpecPath) {
  // pkg.ads -> pkg.ads.mf
  return SpecPath + ".mf";
}

bool writeModuleFile(const std::string &Path,
                     const std::string &UnitName,
                     const std::string &Source,
                     int64_t Timestamp,
                     const LibraryUnit *AST) {
  // Serialize AST to text by re-parsing the source
  // (simpler than implementing a true AST serializer for P7)
  std::string ASTText = Source;

  // Write header
  FILE *F = fopen(Path.c_str(), "w");
  if (!F) {
    std::cerr << "error: cannot write module file '" << Path << "'\n";
    return false;
  }

  uint64_t SourceHash = hashSource(Source);

  // Write magic
  fwrite(MODULE_MAGIC, 1, 8, F);
  // Write version
  fwrite(&MODULE_VERSION, sizeof(uint32_t), 1, F);
  // Write source hash
  fwrite(&SourceHash, sizeof(uint64_t), 1, F);
  // Write timestamp
  fwrite(&Timestamp, sizeof(int64_t), 1, F);
  // Write unit name length
  uint32_t UnitNameLen = UnitName.size();
  fwrite(&UnitNameLen, sizeof(uint32_t), 1, F);
  // Write AST text length
  uint64_t ASTTextLen = ASTText.size();
  fwrite(&ASTTextLen, sizeof(uint64_t), 1, F);
  // Pad header to 64 bytes
  uint8_t Pad[8] = {0};
  fwrite(Pad, 1, 8, F);

  // Write unit name
  fwrite(UnitName.c_str(), 1, UnitNameLen, F);
  fputc('\0', F);

  // Write AST text
  fwrite(ASTText.c_str(), 1, ASTTextLen, F);

  fclose(F);
  return true;
}

std::unique_ptr<LibraryUnit> readModuleFile(const std::string &Path) {
  FILE *F = fopen(Path.c_str(), "r");
  if (!F) {
    return nullptr;
  }

  // Read and verify magic
  uint8_t Magic[8];
  if (fread(Magic, 1, 8, F) != 8 || memcmp(Magic, MODULE_MAGIC, 8) != 0) {
    fclose(F);
    return nullptr;
  }

  // Read version
  uint32_t Version;
  if (fread(&Version, sizeof(uint32_t), 1, F) != 1) {
    fclose(F);
    return nullptr;
  }
  if (Version != MODULE_VERSION) {
    fclose(F);
    return nullptr;
  }

  // Read source hash (we don't use it for now, but read it to advance the stream)
  uint64_t SourceHash;
  if (fread(&SourceHash, sizeof(uint64_t), 1, F) != 1) {
    fclose(F);
    return nullptr;
  }

  // Read timestamp
  int64_t Timestamp;
  if (fread(&Timestamp, sizeof(int64_t), 1, F) != 1) {
    fclose(F);
    return nullptr;
  }

  // Read unit name length
  uint32_t UnitNameLen;
  if (fread(&UnitNameLen, sizeof(uint32_t), 1, F) != 1) {
    fclose(F);
    return nullptr;
  }

  // Read AST text length
  uint64_t ASTTextLen;
  if (fread(&ASTTextLen, sizeof(uint64_t), 1, F) != 1) {
    fclose(F);
    return nullptr;
  }

  // Skip padding
  uint8_t Pad[8];
  if (fread(Pad, 1, 8, F) != 8) {
    fclose(F);
    return nullptr;
  }

  // Read unit name
  std::string UnitName(UnitNameLen, '\0');
  if (fread(&UnitName[0], 1, UnitNameLen, F) != UnitNameLen) {
    fclose(F);
    return nullptr;
  }

  // Read AST text
  std::string ASTText(ASTTextLen, '\0');
  if (fread(&ASTText[0], 1, ASTTextLen, F) != ASTTextLen) {
    fclose(F);
    return nullptr;
  }

  fclose(F);

  // Parse the source text back into a LibraryUnit
  AdaParser Parser(UnitName, ASTText);
  auto Unit = Parser.ParseLibraryUnit();
  if (!Unit) {
    std::cerr << "error: failed to parse module file '" << Path << "'\n";
    return nullptr;
  }

  return Unit;
}

bool moduleFileIsCurrent(const std::string &ModulePath,
                         const std::string &SourcePath) {
  struct stat ModuleStat, SourceStat;
  if (stat(ModulePath.c_str(), &ModuleStat) != 0) {
    return false;
  }
  if (stat(SourcePath.c_str(), &SourceStat) != 0) {
    return false;
  }
  // Module is current if it's newer than the source
  return ModuleStat.st_mtime >= SourceStat.st_mtime;
}

} // namespace clang::ada::serialization
