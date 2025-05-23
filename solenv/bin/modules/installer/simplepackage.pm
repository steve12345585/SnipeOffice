#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# This file incorporates work covered by the following license notice:
#
#   Licensed to the Apache Software Foundation (ASF) under one or more
#   contributor license agreements. See the NOTICE file distributed
#   with this work for additional information regarding copyright
#   ownership. The ASF licenses this file to you under the Apache
#   License, Version 2.0 (the "License"); you may not use this file
#   except in compliance with the License. You may obtain a copy of
#   the License at http://www.apache.org/licenses/LICENSE-2.0 .
#

package installer::simplepackage;

use strict;
use warnings;

use Cwd;
use File::Copy;
use installer::download;
use installer::exiter;
use installer::globals;
use installer::logger;
use installer::strip qw(strip_libraries);
use installer::systemactions;
use installer::worker;

####################################################
# Checking if the simple packager is required.
# This can be achieved by setting the global
# variable SIMPLE_PACKAGE in *.lst file or by
# setting the environment variable SIMPLE_PACKAGE.
####################################################

sub check_simple_packager_project
{
    my ( $allvariables ) = @_;

    if ( $installer::globals::packageformat eq "installed" )
    {
        $installer::globals::is_simple_packager_project = 1;
        $installer::globals::patch_user_dir = 1;
    }
    elsif(( $installer::globals::packageformat eq "archive" ) ||
          ( $installer::globals::packageformat eq "dmg" ) )
    {
        $installer::globals::is_simple_packager_project = 1;
    }
}

####################################################
# Detecting the directory with extensions
####################################################

sub get_extensions_dir
{
    my ( $subfolderdir ) = @_;

    my $extensiondir = $subfolderdir . $installer::globals::separator;
    if ( $installer::globals::officedirhostname ne "" ) { $extensiondir = $extensiondir . $installer::globals::officedirhostname . $installer::globals::separator; }
    my $extensionsdir = $extensiondir . "share" . $installer::globals::separator . "extensions";

    return $extensionsdir;
}

##################################################################
# Collecting all identifier from ulf file
##################################################################

sub get_identifier
{
    my ( $translationfile ) = @_;

    my @identifier = ();

    for ( my $i = 0; $i <= $#{$translationfile}; $i++ )
    {
        my $oneline = ${$translationfile}[$i];

        if ( $oneline =~ /^\s*\[(.+)\]\s*$/ )
        {
            my $identifier = $1;
            push(@identifier, $identifier);
        }
    }

    return \@identifier;
}

##############################################################
# Returning the complete block in all languages
# for a specified string
##############################################################

sub get_language_block_from_language_file
{
    my ($searchstring, $languagefile) = @_;

    my @language_block = ();

    for ( my $i = 0; $i <= $#{$languagefile}; $i++ )
    {
        if ( ${$languagefile}[$i] =~ /^\s*\[\s*$searchstring\s*\]\s*$/ )
        {
            my $counter = $i;

            push(@language_block, ${$languagefile}[$counter]);
            $counter++;

            while (( $counter <= $#{$languagefile} ) && (!( ${$languagefile}[$counter] =~ /^\s*\[/ )))
            {
                push(@language_block, ${$languagefile}[$counter]);
                $counter++;
            }

            last;
        }
    }

    return \@language_block;
}

##############################################################
# Returning a specific language string from the block
# of all translations
##############################################################

sub get_language_string_from_language_block
{
    my ($language_block, $language) = @_;

    my $newstring = "";

    for ( my $i = 0; $i <= $#{$language_block}; $i++ )
    {
        if ( ${$language_block}[$i] =~ /^\s*$language\s*\=\s*\"(.*)\"\s*$/ )
        {
            $newstring = $1;
            last;
        }
    }

    if ( $newstring eq "" )
    {
        $language = "en-US";    # defaulting to english

        for ( my $i = 0; $i <= $#{$language_block}; $i++ )
        {
            if ( ${$language_block}[$i] =~ /^\s*$language\s*\=\s*\"(.*)\"\s*$/ )
            {
                $newstring = $1;
                last;
            }
        }
    }

    return $newstring;
}

########################################################################
# Localizing the script for the Mac Language Pack installer
########################################################################

sub localize_scriptfile
{
    my ($scriptfile, $translationfile, $languagestringref) = @_;

    my $onelanguage = $$languagestringref;
    if ( $onelanguage =~ /^\s*(.*?)_/ ) { $onelanguage = $1; }

    # Analyzing the ulf file, collecting all Identifier
    my $allidentifier = get_identifier($translationfile);

    for ( my $i = 0; $i <= $#{$allidentifier}; $i++ )
    {
        my $identifier = ${$allidentifier}[$i];
        my $language_block = get_language_block_from_language_file($identifier, $translationfile);
        my $newstring = get_language_string_from_language_block($language_block, $onelanguage);

        # removing mask
        $newstring =~ s/\\\'/\'/g;

        replace_one_variable_in_shellscript($scriptfile, $newstring, $identifier);
    }
}

#################################################################################
# Replacing one variable in Mac shell script
#################################################################################

sub replace_one_variable_in_shellscript
{
    my ($scriptfile, $variable, $searchstring) = @_;

    for ( my $i = 0; $i <= $#{$scriptfile}; $i++ )
    {
        ${$scriptfile}[$i] =~ s/\[$searchstring\]/$variable/g;
    }
}

#############################################
# Replacing variables in Mac shell script
#############################################

sub replace_variables_in_scriptfile
{
    my ($scriptfile, $volume_name, $volume_name_app, $allvariables) = @_;

    replace_one_variable_in_shellscript($scriptfile, $volume_name, "FULLPRODUCTNAME" );
    replace_one_variable_in_shellscript($scriptfile, $volume_name_app, "FULLAPPPRODUCTNAME" );
    replace_one_variable_in_shellscript($scriptfile, $allvariables->{'PRODUCTNAME'}, "PRODUCTNAME" );
    replace_one_variable_in_shellscript($scriptfile, $allvariables->{'PRODUCTVERSION'}, "PRODUCTVERSION" );

    my $scriptname = $allvariables->{'BUNDLEIDENTIFIER'};

    replace_one_variable_in_shellscript($scriptfile, $scriptname, "SEARCHSCRIPTNAME" );
}

#############################################
# Creating the "simple" package.
# "zip" for Windows
# "tar.gz" for all other platforms
# additionally "dmg" on macOS
#############################################

sub create_package
{
    my ( $installdir, $archivedir, $packagename, $allvariables, $includepatharrayref, $languagestringref, $format ) = @_;

    installer::logger::print_message( "... creating $installer::globals::packageformat file ...\n" );
    installer::logger::include_header_into_logfile("Creating $installer::globals::packageformat file:");

    # moving dir into temporary directory
    my $pid = $$; # process id
    my $tempdir = $installdir . "_temp" . "." . $pid;
    my $systemcall = "";
    my $from = "";
    my $makesystemcall = 1;
    my $return_to_start = 0;
    installer::systemactions::rename_directory($installdir, $tempdir);

    # creating new directory with original name
    installer::systemactions::create_directory($archivedir);

    my $archive = $archivedir . $installer::globals::separator . $packagename . $format;

    if ( $archive =~ /zip$/ )
    {
        $from = cwd();
        $return_to_start = 1;
        chdir($tempdir);
        $systemcall = "$installer::globals::zippath -qr $archive .";

        # Using Archive::Zip fails because of very long path names below "share/uno_packages/cache"
        # my $packzip = Archive::Zip->new();
        # $packzip->addTree(".");   # after changing into $tempdir
        # $packzip->writeToFileNamed($archive);
        # $makesystemcall = 0;
    }
     elsif ( $archive =~ /dmg$/ )
    {
        my $folder = (( -l "$tempdir/$packagename/Applications" ) or ( -l "$tempdir/$packagename/opt" )) ? $packagename : "\.";

        if ( $allvariables->{'PACK_INSTALLED'} ) {
            $folder = $packagename;
        }

        my $volume_name = $allvariables->{'PRODUCTNAME'};
        my $volume_name_classic = $allvariables->{'PRODUCTNAME'} . ' ' . $allvariables->{'PRODUCTVERSION'};
        my $volume_name_classic_app = $volume_name;  # "app" should not contain version number
        if ( $allvariables->{'DMG_VOLUMEEXTENSION'} ) {
            $volume_name = $volume_name . ' ' . $allvariables->{'DMG_VOLUMEEXTENSION'};
            $volume_name_classic = $volume_name_classic . ' ' . $allvariables->{'DMG_VOLUMEEXTENSION'};
            $volume_name_classic_app = $volume_name_classic_app . ' ' . $allvariables->{'DMG_VOLUMEEXTENSION'};
        }

        my $sla = 'sla.r';
        my $ref = "";

        if ( ! $allvariables->{'HIDELICENSEDIALOG'} )
        {
            installer::scriptitems::get_sourcepath_from_filename_and_includepath( \$sla, $includepatharrayref, 0);
        }

        my $localtempdir = $tempdir;

        if (( $installer::globals::languagepack ) || ( $installer::globals::helppack ))
        {
            # LanguagePack and HelpPack files are collected in $srcfolder, packaged into
            # tarball.tar.bz2 and finally the Language Pack.app is assembled in $appfolder
            $localtempdir = "$tempdir/$packagename";
            my $srcfolder = $localtempdir . "/" . $volume_name_classic_app . "\.app";

            $volume_name             .= " " . $$languagestringref . " Language Pack";
            $volume_name_classic     .= " Language Pack";
            $volume_name_classic_app .= " Language Pack";

            my $appfolder = $localtempdir . "/" . $volume_name_classic_app . "\.app";
            my $contentsfolder = $appfolder . "/Contents";
            my $tarballname = "tarball.tar.bz2";

            my $localfrom = cwd();
            chdir $srcfolder;

            $systemcall = "tar -cjf $tarballname Contents/";

            print "... $systemcall ...\n";
            my $localreturnvalue = system($systemcall);
            my $infoline = "Systemcall: $systemcall\n";
            push( @installer::globals::logfileinfo, $infoline);

            if ($localreturnvalue)
            {
                $infoline = "ERROR: Could not execute \"$systemcall\"!\n";
                push( @installer::globals::logfileinfo, $infoline);
            }
            else
            {
                $infoline = "Success: Executed \"$systemcall\" successfully!\n";
                push( @installer::globals::logfileinfo, $infoline);
            }

            my $sourcefile = $srcfolder . "/" . $tarballname;
            my $destfile = $contentsfolder . "/Resources/" . $tarballname;

            installer::systemactions::remove_complete_directory($appfolder);
            installer::systemactions::create_directory($appfolder);
            installer::systemactions::create_directory($contentsfolder);
            installer::systemactions::create_directory($contentsfolder . "/Resources");

            installer::systemactions::copy_one_file($sourcefile, $destfile);
            installer::systemactions::remove_complete_directory($srcfolder);

            # Copy two files into installation set next to the tar ball
            # 1. "osx_install.applescript"
            # 2 "OpenOffice.org Languagepack"

            my $scriptrealfilename = "osx_install.applescript";
            my $scriptfilename = "";
            if ( $installer::globals::languagepack ) { $scriptfilename = "osx_install_languagepack.applescript"; }
            if ( $installer::globals::helppack ) { $scriptfilename = "osx_install_helppack.applescript"; }
            my $scripthelperfilename = $ENV{'SRCDIR'} . "/setup_native/scripts/mac_install.script";
            my $scripthelperrealfilename = $volume_name_classic_app;

            # Finding both files in source tree

            my $scriptref = $ENV{'SRCDIR'} . "/setup_native/scripts/" . $scriptfilename;
            if (! -f $scriptref) { installer::exiter::exit_program("ERROR: Could not find Apple script $scriptfilename ($scriptref)!", "create_package"); }
            if (! -f $scripthelperfilename) { installer::exiter::exit_program("ERROR: Could not find Apple script $scripthelperfilename!", "create_package"); }

            $scriptfilename = $contentsfolder . "/Resources/" . $scriptrealfilename;
            $scripthelperrealfilename = $contentsfolder . "/" . $scripthelperrealfilename;

            installer::systemactions::copy_one_file($scriptref, $scriptfilename);
            installer::systemactions::copy_one_file($scripthelperfilename, $scripthelperrealfilename);

            # Replacing variables in script $scriptfilename
            # Localizing script $scriptfilename
            my $scriptfilecontent = installer::files::read_file($scriptfilename);
            my $translationfilecontent = installer::files::read_file($installer::globals::macinstallfilename);
            localize_scriptfile($scriptfilecontent, $translationfilecontent, $languagestringref);

            replace_variables_in_scriptfile($scriptfilecontent, $volume_name_classic, $volume_name_classic_app, $allvariables);
            installer::files::save_file($scriptfilename, $scriptfilecontent);

            chmod 0775, $scriptfilename;
            chmod 0775, $scripthelperrealfilename;

            # Copy also Info.plist and icon file
            # Finding both files in source tree
            my $iconfile = "ooo3_installer.icns";
            my $iconfileref = $ENV{'SRCDIR'} . "/setup_native/source/mac/" . $iconfile;
            if (! -f $iconfileref) { installer::exiter::exit_program("ERROR: Could not find Apple script icon file $iconfile ($iconfileref)!", "create_package"); }
            my $subdir = $contentsfolder . "/" . "Resources";
            if ( ! -d $subdir ) { installer::systemactions::create_directory($subdir); }
            $destfile = $subdir . "/" . $iconfile;
            installer::systemactions::copy_one_file($iconfileref, $destfile);

            my $infoplistfile = $ENV{'SRCDIR'} . "/setup_native/source/mac/Info.plist.langpack";
            if (! -f $infoplistfile) { installer::exiter::exit_program("ERROR: Could not find Apple script Info.plist: $infoplistfile!", "create_package"); }
            $destfile = "$contentsfolder/Info.plist";
            # Replacing variables in Info.plist
            $scriptfilecontent = installer::files::read_file($infoplistfile);

            replace_one_variable_in_shellscript($scriptfilecontent, $volume_name_classic_app, "FULLAPPPRODUCTNAME" ); # OpenOffice.org Language Pack
            replace_one_variable_in_shellscript($scriptfilecontent, $ENV{'MACOSX_BUNDLE_IDENTIFIER'}, "BUNDLEIDENTIFIER" );
            installer::files::save_file($destfile, $scriptfilecontent);

            chdir $localfrom;

            if ( $ENV{'MACOSX_CODESIGNING_IDENTITY'} ) {
                my $lp_sign = "codesign --verbose --sign $ENV{'MACOSX_CODESIGNING_IDENTITY'} --deep '$appfolder'" ;
                my $output = `$lp_sign 2>&1`;
                unless ($?) {
                    $infoline = "Success: \"$lp_sign\" executed successfully!\n";
                } else {
                    $infoline = "ERROR: Could not codesign the languagepack using \"$lp_sign\"!\n$output\n";
                }
                push( @installer::globals::logfileinfo, $infoline);
            }
        }
        elsif ($volume_name_classic_app eq 'LibreOffice' || $volume_name_classic_app eq 'LibreOfficeDev')
        {
            my $subdir = "$tempdir/$packagename/$volume_name_classic_app.app/Contents/Resources";
            if ( ! -d $subdir ) { installer::systemactions::create_directory($subdir); }
            # For non-release builds where no identity is, set entitlements
            # to allow Xcode's Instruments application to connect to the
            # application
            if ( $ENV{'MACOSX_CODESIGNING_IDENTITY'} || !$ENV{'ENABLE_RELEASE_BUILD'} )
            {
                $systemcall = "$ENV{'SRCDIR'}/solenv/bin/macosx-codesign-app-bundle $localtempdir/$folder/$volume_name_classic_app.app";
                print "... $systemcall ...\n";
                my $infoline = "Systemcall: $systemcall\n";
                push( @installer::globals::logfileinfo, $infoline);
                my $output = `$systemcall 2>&1`;
                if ($?)
                {
                    $infoline = "ERROR: Could not execute \"$systemcall\"!\n$output\n";
                    push( @installer::globals::logfileinfo, $infoline);
                }
                else
                {
                    $infoline = "Success: Executed \"$systemcall\" successfully!\n";
                    push( @installer::globals::logfileinfo, $infoline);
                }
            }
        }
        elsif ($volume_name_classic_app eq 'LibreOffice SDK' || $volume_name_classic_app eq 'LibreOfficeDev SDK')
        {
            if ( $ENV{'MACOSX_CODESIGNING_IDENTITY'} )
            {
                my $sdkbindir = "$localtempdir/$folder/$allvariables->{'PRODUCTNAME'}$allvariables->{'PRODUCTVERSION'}_SDK/bin";
                opendir(my $dh, $sdkbindir);
                foreach my $sdkbinary (readdir $dh) {
                    next unless -f "$sdkbindir/$sdkbinary";
                    $systemcall = "codesign --force --verbose --options=runtime --identifier='$ENV{MACOSX_BUNDLE_IDENTIFIER}.$sdkbinary' --sign '$ENV{MACOSX_CODESIGNING_IDENTITY}' --entitlements $ENV{BUILDDIR}/hardened_runtime.xcent $sdkbindir/$sdkbinary > /tmp/codesign_losdk_$sdkbinary.log 2>&1";
                    print "... $systemcall ...\n";
                    my $returnvalue = system($systemcall);
                    my $infoline = "Systemcall: $systemcall\n";
                    push( @installer::globals::logfileinfo, $infoline);

                    if ($returnvalue)
                    {
                        $infoline = "ERROR: Could not execute \"$systemcall\"!\n";
                        push( @installer::globals::logfileinfo, $infoline);
                    }
                    else
                    {
                        $infoline = "Success: Executed \"$systemcall\" successfully!\n";
                        push( @installer::globals::logfileinfo, $infoline);
                        unlink "/tmp/codesign_losdk_$sdkbinary.log";
                    }
                }
                closedir($dh);
            }
        }
        my $megabytes = 1500;
        $megabytes = 3000 if $ENV{'ENABLE_DEBUG'};

        # tdf#151341 Use lzfse compression instead of bzip2
        # Several users reported that copying the LibreOffice.app package
        # in the Finder from a .dmg file compressed with lzfse compression
        # copies the package several times faster than from a .dmg compressed
        # with bzip2 compression.
        # On a mid-2015 Intel MacBook Pro running macOS, copying in the Finder
        # was at least 5 times faster with lzfse than with bzip2. Also, the
        # hdiutil man page as of macOS Monterey 12.6.2 has marked bzip2 as
        # deprecated. lzfse is marked as supported since macOS El Capitan 10.11
        # so this change appears safe.
        # The one thing that bzip2 has is better compression so a .dmg with
        # bzip2 should be smaller than with lzfse. A .dmg built from a debug
        # build was 262M with bzip2 and 273MB for lzfse. So it appears that
        # lzfse creates .dmg files that are only 4% or 5% larger than bzip2.
        $systemcall = "cd $localtempdir && hdiutil create -megabytes $megabytes -srcfolder $folder $archive -ov -fs HFS+ -volname \"$volume_name\" -format ULFO";
        if (( $ref ne "" ) && ( $$ref ne "" ) && system("hdiutil 2>&1 | grep unflatten") == 0) {
            $systemcall .= " && hdiutil unflatten $archive && Rez -a $$ref -o $archive && hdiutil flatten $archive &&";
        }
    }
    else
    {
        # use fakeroot (only required for Solaris and Linux)
        my $fakerootstring = "";
        if (( $installer::globals::issolarisbuild ) || ( $installer::globals::islinuxbuild ))
        {
            $fakerootstring = "fakeroot";
        }

        $systemcall = "cd $tempdir; $fakerootstring tar -cf - . | $installer::globals::packertool > $archive";
    }

    if ( $makesystemcall )
    {
        print "... $systemcall ...\n";
        my $systemcall_output = `$systemcall`;
        my $returnvalue = $? >> 8;
        my $infoline = "Systemcall: $systemcall\n";
        push( @installer::globals::logfileinfo, $infoline);

        if ($returnvalue)
        {
            $infoline = "ERROR: Could not execute \"$systemcall\" - exitcode: $returnvalue - output:\n$systemcall_output\n";
            push( @installer::globals::logfileinfo, $infoline);
        }
        else
        {
            $infoline = "Success: Executed \"$systemcall\" successfully!\n";
            push( @installer::globals::logfileinfo, $infoline);
        }
    }

    if ( $return_to_start ) { chdir($from); }

    print "... removing $tempdir ...\n";
    installer::systemactions::remove_complete_directory($tempdir);
}

####################################################
# Main method for creating the simple package
# installation sets
####################################################

sub create_simple_package
{
    my ( $filesref, $dirsref, $scpactionsref, $linksref, $unixlinksref, $loggingdir, $languagestringref, $shipinstalldir, $allsettingsarrayref, $allvariables, $includepatharrayref ) = @_;

    # Creating directories

    my $current_install_number = "";
    my $infoline = "";

    installer::logger::print_message( "... creating installation directory ...\n" );
    installer::logger::include_header_into_logfile("Creating installation directory");

    $installer::globals::csp_installdir = installer::worker::create_installation_directory($shipinstalldir, $languagestringref, \$current_install_number);
    $installer::globals::csp_installlogdir = installer::systemactions::create_directory_next_to_directory($installer::globals::csp_installdir, "log");

    my $installdir = $installer::globals::csp_installdir;
    my $installlogdir = $installer::globals::csp_installlogdir;

    # Setting package name (similar to the download name)
    my $packagename = "";

    if ( $installer::globals::packageformat eq "archive"  ||
        $installer::globals::packageformat eq "dmg" )
    {
        $installer::globals::csp_languagestring = $$languagestringref;

        my $locallanguage = $installer::globals::csp_languagestring;

        $packagename = installer::download::set_download_filename(\$locallanguage, $allvariables);
    }

    # Work around Windows problems with long pathnames (see issue 50885) by
    # putting the to-be-archived installation tree into the temp directory
    # instead of the module output tree (unless LOCALINSTALLDIR dictates
    # otherwise, anyway); can be removed once issue 50885 is fixed:
    my $tempinstalldir = $installdir;
    if ( $installer::globals::iswindowsbuild &&
         $installer::globals::packageformat eq "archive" &&
         !$installer::globals::localinstalldirset )
    {
        $tempinstalldir = File::Temp::tempdir;
    }

    # Creating subfolder in installdir, which shall become the root of package or zip file
    my $subfolderdir = "";
    if ( $packagename ne "" ) { $subfolderdir = $tempinstalldir . $installer::globals::separator . $packagename; }
    else { $subfolderdir = $tempinstalldir; }

    if ( ! -d $subfolderdir ) { installer::systemactions::create_directory($subfolderdir); }

    # Create directories, copy files and ScpActions

    installer::logger::print_message( "... creating directories ...\n" );
    installer::logger::include_header_into_logfile("Creating directories:");

    for ( my $i = 0; $i <= $#{$dirsref}; $i++ )
    {
        my $onedir = ${$dirsref}[$i];

        if ( $onedir->{'HostName'} )
        {
            my $destdir = $subfolderdir . $installer::globals::separator . $onedir->{'HostName'};

            if ( ! -d $destdir )
            {
                if ( $^O =~ /cygwin/i ) # Cygwin performance check
                {
                    $infoline = "Try to create directory $destdir\n";
                    push(@installer::globals::logfileinfo, $infoline);
                    # Directories in $dirsref are sorted and all parents were added -> "mkdir" works without parent creation!
                    if ( ! ( -d $destdir )) { mkdir($destdir, 0775); }
                }
                else
                {
                    installer::systemactions::create_directory_structure($destdir);
                }
            }
        }
    }

    # stripping files ?!
    if (( $installer::globals::strip ) && ( ! $installer::globals::iswindowsbuild )) { strip_libraries($filesref, $languagestringref); }

    # copy Files
    installer::logger::print_message( "... copying files ...\n" );
    installer::logger::include_header_into_logfile("Copying files:");

    for ( my $i = 0; $i <= $#{$filesref}; $i++ )
    {
        my $onefile = ${$filesref}[$i];

        if (( $onefile->{'Styles'} ) && ( $onefile->{'Styles'} =~ /\bBINARYTABLE_ONLY\b/ )) { next; }

        my $source = $onefile->{'sourcepath'};
        my $destination = $onefile->{'destination'};
        $destination = $subfolderdir . $installer::globals::separator . $destination;

        # Replacing $$ by $ is necessary to install files with $ in its name (back-masquerading)
        # Otherwise, the following shell command does not work and the file list is not correct
        $source =~ s/\$\$/\$/;
        $destination =~ s/\$\$/\$/;

        if ( $^O =~ /cygwin/i ) # Cygwin performance, do not use copy_one_file. "chmod -R" at the end
        {
            my $copyreturn = copy($source, $destination);

            if ($copyreturn)
            {
                $infoline = "Copy: $source to $destination\n";
            }
            else
            {
                $infoline = "ERROR: Could not copy $source to $destination $!\n";
            }

            push(@installer::globals::logfileinfo, $infoline);
        }
        else
        {
            installer::systemactions::copy_one_file($source, $destination);

            if ( ! $installer::globals::iswindowsbuild )
            {
                # see issue 102274
                if ( $onefile->{'UnixRights'} )
                {
                    if ( ! -l $destination ) # that would be rather pointless
                    {
                        chmod oct($onefile->{'UnixRights'}), $destination;
                    }
                }
            }
        }
    }

    # creating Links

    installer::logger::print_message( "... creating links ...\n" );
    installer::logger::include_header_into_logfile("Creating links:");

    for ( my $i = 0; $i <= $#{$linksref}; $i++ )
    {
        my $onelink = ${$linksref}[$i];

        my $destination = $onelink->{'destination'};
        $destination = $subfolderdir . $installer::globals::separator . $destination;
        my $destinationfile = $onelink->{'destinationfile'};

        my $localcall = "ln -sf \'$destinationfile\' \'$destination\' \>\/dev\/null 2\>\&1";
        system($localcall);

        $infoline = "Creating link: \"ln -sf $destinationfile $destination\"\n";
        push(@installer::globals::logfileinfo, $infoline);
    }

    for ( my $i = 0; $i <= $#{$unixlinksref}; $i++ )
    {
        my $onelink = ${$unixlinksref}[$i];

        my $target = $onelink->{'Target'};
        my $destination = $subfolderdir . $installer::globals::separator . $onelink->{'destination'};

        my @localcall = ('ln', '-sf', $target, $destination);
        system(@localcall) == 0 or die "system @localcall failed: $?";

        $infoline = "Creating Unix link: \"@localcall\"\n";
        push(@installer::globals::logfileinfo, $infoline);
    }

    # Setting privileges for cygwin globally

    if ( $^O =~ /cygwin/i )
    {
        installer::logger::print_message( "... changing privileges in $subfolderdir ...\n" );
        installer::logger::include_header_into_logfile("Changing privileges in $subfolderdir:");

        my $localcall = "chmod -R 755 " . "\"" . $subfolderdir . "\"";
        system($localcall);
    }

    installer::logger::print_message( "... removing superfluous directories ...\n" );
    installer::logger::include_header_into_logfile("Removing superfluous directories:");

    my $extensionfolder = get_extensions_dir($subfolderdir);
    installer::systemactions::remove_empty_dirs_in_folder($extensionfolder);

    if ( $installer::globals::ismacbuild )
    {
        installer::worker::put_scpactions_into_installset("$installdir/$packagename");
    }

    # Creating archive file
    if ( $installer::globals::packageformat eq "archive" )
    {
        create_package($tempinstalldir, $installdir, $packagename, $allvariables, $includepatharrayref, $languagestringref, $installer::globals::archiveformat);
    }
    elsif ( $installer::globals::packageformat eq "dmg" )
    {
        create_package($installdir, $installdir, $packagename, $allvariables, $includepatharrayref, $languagestringref, ".dmg");
    }

    # Analyzing the log file

    installer::worker::clean_output_tree(); # removing directories created in the output tree
    installer::worker::analyze_and_save_logfile($loggingdir, $installdir, $installlogdir, $allsettingsarrayref, $languagestringref, $current_install_number);
}

1;

# vim: set shiftwidth=4 softtabstop=4 expandtab:
