/***************************************************************************

    climess.c

    Command-line interface frontend for MESS.

***************************************************************************/

#include "emu.h"
#include "climess.h"
#include "output.h"
#include "hash.h"
#include "xmlfile.h"


/*-------------------------------------------------
    mess_display_help - display MESS help to
    standard output
-------------------------------------------------*/

void mess_display_help(void)
{
	mame_printf_info(
		"MESS v%s\n"
		"Multi Emulator Super System - Copyright (C) 1997-2009 by the MESS Team\n"
		"MESS is based on MAME Source code\n"
		"Copyright (C) 1997-2009 by Nicola Salmoria and the MAME Team\n\n",
		build_version);
	mame_printf_info("%s\n", mess_disclaimer);
	mame_printf_info(
		"Usage:  MESS <system> <device> <software> <options>\n"
		"\n"
		"        MESS -showusage    for a brief list of options\n"
		"        MESS -showconfig   for a list of configuration options\n"
		"        MESS -listdevices  for a full list of supported devices\n"
		"        MESS -createconfig to create a mess.ini\n"
		"\n"
		"See config.txt and windows.txt for usage instructions.\n");
}

/*-------------------------------------------------
    cli_info_listdevices - output the list of
    devices referenced by a given game or set of
    games
-------------------------------------------------*/

int info_listmedia(core_options *options, const char *gamename)
{
	int count = 0, devcount;
	int drvindex;
	machine_config *config;
	device_config *dev;
	image_device_info info;
	const char *src;
	const char *driver_name;
	const char *name;
	const char *shortname;
	char paren_shortname[16];

	printf(" SYSTEM      DEVICE NAME (brief)   IMAGE FILE EXTENSIONS SUPPORTED    \n");
	printf("----------  --------------------  ------------------------------------\n");

	/* iterate over drivers */
	for (drvindex = 0; drivers[drvindex] != NULL; drvindex++)
		if (mame_strwildcmp(gamename, drivers[drvindex]->name) == 0)
		{
			/* allocate the machine config */
			config = machine_config_alloc(drivers[drvindex]->machine_config);

			driver_name = drivers[drvindex]->name;

			devcount = 0;

			for (dev = config->devicelist.first(); dev != NULL; dev = dev->next)
			{
				if (is_image_device(dev))
				{
					info = image_device_getinfo(config, dev);

					src = info.file_extensions;
					name = info.instance_name;
					shortname = info.brief_instance_name;

					sprintf(paren_shortname, "(%s)", shortname);

					printf("%-13s%-12s%-8s   ", driver_name, name, paren_shortname);
					driver_name = " ";

					while (src && *src)
					{
						printf(".%-5s", src);
						src += strlen(src) + 1;
						devcount++;
					}
					printf("\n");
				}
			}
			if (!devcount)
				printf("%-13s(none)\n",driver_name);

			count++;
			machine_config_free(config);
		}

	if (!count)
		printf("There are no Computers or Consoles named %s\n", gamename);

	return (count > 0) ? MAMERR_NONE : MAMERR_NO_SUCH_GAME;
}


/*-------------------------------------------------
    info_listsoftware - output the list of
    software supported by a given game or set of
    games
	TODO: Add all information read from the source files
    Possible improvement: use a sorted list for
        identifying duplicate lists.
-------------------------------------------------*/

int info_listsoftware(core_options *options, const char *gamename)
{
	FILE *out = stdout;
	int nr_lists = 0;
	char ** lists = NULL;
	int list_idx = 0;

	/* First determine the maximum number of lists we might encounter */
	for ( int drvindex = 0; drivers[drvindex] != NULL; drvindex++ )
	{
		if ( mame_strwildcmp( gamename, drivers[drvindex]->name ) == 0 )
		{
			/* allocate the machine config */
			machine_config *config = machine_config_alloc( drivers[drvindex]->machine_config );

			for (const device_config *dev = config->devicelist.first(); dev != NULL; dev = dev->next)
			{
				if ( ! strcmp( dev->tag(), __SOFTWARE_LIST_TAG ) )
				{
					software_list_config *swlist = (software_list_config *)dev->inline_config;

					for ( int i = 0; i < DEVINFO_STR_SWLIST_MAX - DEVINFO_STR_SWLIST_0; i++ )
					{
						if ( swlist->list_name[i] && *swlist->list_name[i] )
							nr_lists++;
					}
				}
			}

			/* free the machine config */
			machine_config_free( config );
		}
	}

	lists = global_alloc_array( char *, nr_lists );

	fprintf( out,
			"<?xml version=\"1.0\"?>\n"
			"<!ELEMENT softwarelists (softwarelist*)\n"
			"\t<!ELEMENT softwarelist (software+)>\n"
			"\t\t<!ATTLIST softwarelist name CDATA #REQUIRED>\n"
			"\t\t<!ATTLIST softwarelist description CDATA #IMPLIED>\n"
			"\t\t<!ELEMENT software (description, year?, publisher, part*)>\n"
			"\t\t\t<!ATTLIST software name CDATA #REQUIRED>\n"
			"\t\t\t<!ATTLIST software cloneof CDATA #IMPLIED>\n"
			"\t\t\t<!ATTLIST software supported (yes|partial|no) \"yes\">\n"
			"\t\t\t<!ELEMENT description (#PCDATA)>\n"
			"\t\t\t<!ELEMENT year (#PCDATA)>\n"
			"\t\t\t<!ELEMENT publisher (#PCDATA)>\n"
			"\t\t\t<!ELEMENT part (dataarea*)>\n"
			"\t\t\t\t<!ATTLIST part name CDATA #REQUIRED>\n"
			"\t\t\t\t<!ATTLIST part interface CDATA #REQUIRED>\n"
			"\t\t\t\t<!ATTLIST part feature CDATA #IMPLIED>\n"
			"\t\t\t\t<!ELEMENT dataarea (rom*)>\n"
			"\t\t\t\t\t<!ATTLIST dataarea name CDATA #REQUIRED>\n"
			"\t\t\t\t\t<!ATTLIST dataarea size CDATA #REQUIRED>\n"
			"\t\t\t\t\t<!ATTLIST dataarea databits (8|16|32|64) \"8\">\n"
			"\t\t\t\t\t<!ATTLIST dataarea endian (big|little) \"little\">\n"
			"\t\t\t\t\t<!ELEMENT rom EMPTY>\n"
			"\t\t\t\t\t\t<!ATTLIST rom name CDATA #IMPLIED>\n"
			"\t\t\t\t\t\t<!ATTLIST rom size CDATA #REQUIRED>\n"
			"\t\t\t\t\t\t<!ATTLIST rom crc CDATA #IMPLIED>\n"
			"\t\t\t\t\t\t<!ATTLIST rom md5 CDATA #IMPLIED>\n"
			"\t\t\t\t\t\t<!ATTLIST rom sha1 CDATA #IMPLIED>\n"
			"\t\t\t\t\t\t<!ATTLIST rom offset CDATA #IMPLIED>\n"
			"\t\t\t\t\t\t<!ATTLIST rom status (baddump|nodump|good) \"good\">\n"
			"\t\t\t\t\t\t<!ATTLIST rom loadflag (load16_byte|load16_word|load16_word_swap|load32_byte|load32_word|load32_word_swap|load32_dword|load64_word|load64_word_swap|reload) #IMPLIED>\n"
			"<softwarelists>\n"
	);

	for ( int drvindex = 0; drivers[drvindex] != NULL; drvindex++ )
	{
		if ( mame_strwildcmp( gamename, drivers[drvindex]->name ) == 0 )
		{
			/* allocate the machine config */
			machine_config *config = machine_config_alloc( drivers[drvindex]->machine_config );

			for (const device_config *dev = config->devicelist.first(); dev != NULL; dev = dev->next)
			{
				if ( ! strcmp( dev->tag(), __SOFTWARE_LIST_TAG ) )
				{
					software_list_config *swlist = (software_list_config *)dev->inline_config;

					for ( int i = 0; i < DEVINFO_STR_SWLIST_MAX - DEVINFO_STR_SWLIST_0; i++ )
					{
						if ( swlist->list_name[i] && *swlist->list_name[i] )
						{
							software_list *list = software_list_open( options, swlist->list_name[i], FALSE, NULL );

							if ( list )
							{
								/* Verify if we have encountered this list before */
								bool seen_before = false;
								for ( int l = 0; l < list_idx && !seen_before; l++ )
								{
									if ( ! strcmp( swlist->list_name[i], lists[l] ) )
									{
										seen_before = true;
									}
								}

								if ( ! seen_before )
								{
									lists[list_idx] = core_strdup( swlist->list_name[i] );
									list_idx++;

									fprintf(out, "\t<softwarelist name=\"%s\">\n", swlist->list_name[i] );

									for ( software_info *swinfo = software_list_first( list ); swinfo != NULL; swinfo = software_list_next( list ) )
									{
										fprintf( out, "\t\t<software name=\"%s\"", swinfo->shortname );
										if ( swinfo->supported == SOFTWARE_SUPPORTED_PARTIAL )
											fprintf( out, " supported=\"partial\"" );
										if ( swinfo->supported == SOFTWARE_SUPPORTED_NO )
											fprintf( out, " supported=\"no\"" );
										fprintf( out, ">\n" );
										fprintf( out, "\t\t\t<description>%s</description>\n", xml_normalize_string(swinfo->longname) );
										fprintf( out, "\t\t\t<year>%s</year>\n", xml_normalize_string( swinfo->year ) );
										fprintf( out, "\t\t\t<publisher>%s</publisher>\n", xml_normalize_string( swinfo->publisher ) );

										for ( software_part *part = software_find_part( swinfo, NULL, NULL ); part != NULL; part = software_part_next( part ) )
										{
											fprintf( out, "\t\t\t<part name=\"%s\"", part->name );
											if ( part->interface_ )
												fprintf( out, " interface=\"%s\"", part->interface_ );
											if ( part->feature )
												fprintf( out, " features=\"%s\"", part->feature );
											fprintf( out, ">\n");

											/* TODO: display rom region information */
											for ( const rom_entry *region = part->romdata; region; region = rom_next_region( region ) )
											{
												fprintf( out, "\t\t\t\t<dataarea name=\"%s\" size=\"%x\">\n", ROMREGION_GETTAG(region), ROMREGION_GETLENGTH(region) );

												for ( const rom_entry *rom = rom_first_file( region ); rom; rom = rom_next_file( rom ) )
												{
													if ( ROMENTRY_ISFILE(rom) )
													{
														fprintf( out, "\t\t\t\t\t<rom name=\"%s\" size=\"%d\"", ROM_GETNAME(rom), ROM_GETLENGTH(rom) );

														/* dump checksum information only if there is a known dump */
														if (!hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_NO_DUMP))
														{
															char checksum[HASH_BUF_SIZE];
															int hashtype;

															/* iterate over hash function types and print out their values */
															for (hashtype = 0; hashtype < HASH_NUM_FUNCTIONS; hashtype++)
																if (hash_data_extract_printable_checksum(ROM_GETHASHDATA(rom), 1 << hashtype, checksum))
																	fprintf(out, " %s=\"%s\"", hash_function_name(1 << hashtype), checksum);
														}

														fprintf( out, " offset=\"%x\"", ROM_GETOFFSET(rom) );

														if ( hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_BAD_DUMP) )
															fprintf( out, " status=\"baddump\"" );
														if ( hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_NO_DUMP) )
															fprintf( out, " status=\"nodump\"" );

														fprintf( out, "/>\n" );
													}
												}

												fprintf( out, "\t\t\t\t</datearea>\n" );
											}

											fprintf( out, "\t\t\t</part>\n" );
										}

										fprintf( out, "\t\t</software>\n" );
									}

									fprintf(out, "\t</softwarelist>\n" );
								}

								software_list_close( list );
							}
						}
					}
				}
			}

			machine_config_free( config );
		}
	}

	fprintf( out, "</softwarelists>\n" );

	global_free( lists );

	return MAMERR_NONE;
}


/*-------------------------------------------------
    match_roms - scan for a matching software ROM by hash
-------------------------------------------------*/
void mess_match_roms(core_options *options, const char *hash, int length, int *found)
{
	int drvindex;

	/* iterate over drivers */
	for (drvindex = 0; drivers[drvindex] != NULL; drvindex++)
	{
		machine_config *config = machine_config_alloc(drivers[drvindex]->machine_config);

		for (const device_config *dev = config->devicelist.first(); dev != NULL; dev = dev->next)
		{
			if ( ! strcmp( dev->tag(), __SOFTWARE_LIST_TAG ) )
			{
				software_list_config *swlist = (software_list_config *)dev->inline_config;

				for ( int i = 0; i < DEVINFO_STR_SWLIST_MAX - DEVINFO_STR_SWLIST_0; i++ )
				{
					if ( swlist->list_name[i] )
					{
						software_list *list = software_list_open( options, swlist->list_name[i], FALSE, NULL );

						for ( software_info *swinfo = software_list_first( list ); swinfo != NULL; swinfo = software_list_next( list ) )
						{
							for ( software_part *part = software_find_part( swinfo, NULL, NULL ); part != NULL; part = software_part_next( part ) )
							{
								for ( const rom_entry *region = part->romdata; region; region = rom_next_region(region) )
								{
									for ( const rom_entry *rom = rom_first_file(region); rom; rom = rom_next_file(rom) )
									{
										if ( hash_data_is_equal(hash, ROM_GETHASHDATA(rom), 0) )
										{
											int baddump = hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_BAD_DUMP);

											/* output information about the match */
											if (*found != 0)
												mame_printf_info("                    ");
											mame_printf_info("= %s%-20s  %s:%s %s\n", baddump ? "(BAD) " : "", ROM_GETNAME(rom), swlist->list_name[i], swinfo->shortname, swinfo->longname);
											(*found)++;
										}
									}
								}
							}
						}

						software_list_close( list );
					}
				}
			}
		}

		machine_config_free(config);
	}
}

