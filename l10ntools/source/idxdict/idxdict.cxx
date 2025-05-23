/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <cerrno>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <stdlib.h>
#include <string.h>

const int MAXLINE = 1024*64;

int main(int argc, char *argv[])
{
        if (argc != 3 || strcmp(argv[1],"-o"))
        {
                std::cout << "Usage: idxdict -o outputfile < input\n";
                ::exit(99);
        }
        // This call improves performance by approx 5x
        std::ios_base::sync_with_stdio(false);

        const char * outputFile(argv[2]);
        char inputBuffer[MAXLINE];
        std::multimap<std::string, size_t> entries;
        std::multimap<std::string,size_t>::iterator ret(entries.begin());

        std::cin.getline(inputBuffer, MAXLINE);
        const std::string encoding(inputBuffer);
        size_t currentOffset(encoding.size()+1);
        while (true)
        {
                // Extract the next word, but not the entry count
                std::cin.getline(inputBuffer, MAXLINE, '|');

                if (std::cin.eof()) break;

                std::string word(inputBuffer);
                ret = entries.insert(ret, std::pair<std::string, size_t>(word, currentOffset));
                currentOffset += word.size() + 1;
                // Next is the entry count
                std::cin.getline(inputBuffer, MAXLINE);
                if (!std::cin.good())
                {
                        std::cerr << "Unable to read entry - insufficient buffer?.\n";
                        exit(99);
                }
                currentOffset += strlen(inputBuffer)+1;
                char * endptr;
                errno = 0;
                int entryCount(strtol(inputBuffer, &endptr, 10));
                if (errno != 0 || endptr == inputBuffer || *endptr != '\0')
                {
                    std::cerr
                        << "Unable to read count from \"" << inputBuffer
                        << "\" input.\n";
                    exit(99);
                }
                for (int i(0); i < entryCount; ++i)
                {
                        std::cin.getline(inputBuffer, MAXLINE);
                        currentOffset += strlen(inputBuffer)+1;
                }
        }

        // Use binary mode to prevent any translation of LF to CRLF on Windows
        std::ofstream outputStream(outputFile, std::ios_base::binary| std::ios_base::trunc|std::ios_base::out);
        if (!outputStream.is_open())
        {
                std::cerr << "Unable to open output file " << outputFile << std::endl;
                ::exit(99);
        }

        outputStream << encoding << '\n' << entries.size() << '\n';

        for (auto const& entry : entries)
        {
                outputStream << entry.first << '|' << entry.second << '\n';
        }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
