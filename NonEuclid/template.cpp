#include "template.h"

#include <boost/regex.hpp>

#include <boost/algorithm/string/replace.hpp>
#include <boost/foreach.hpp>


using namespace boost;
using namespace templating;

#define foreach BOOST_FOREACH

boost::regex tag_regex("\\{{1,2}(.*?)\\}{1,2}");

boost::regex if_regex("%\\s*if(.*)%");

boost::regex for_regex("%\\s*for\\s*(\\S*)\\s*in\\s*(\\S*)\\s*%");

boost::regex variable_regex("\\s*(\\S*)\\s*");


boost::regex fortag_regex("\\{%\\s*for\\s*(\\S*)\\s*in\\s*(\\S*)\\s*%\\}");
boost::regex endfortag_regex("\\{%\\s*endfor\\s*%\\}");



//A function that recursively expands the template
std::string templating::expand( std::string code, Node* rootnode )
{
	//Make a copy of the string
	std::string result = code;

	std::string::const_iterator start, end;
	start = code.begin();
	end = code.end();

	boost::match_results<std::string::const_iterator> what;
	boost::match_flag_type flags = boost::match_default;

	while( regex_search(start, end, what, tag_regex, flags) )
	{
		//Get the entire match for the regex
		std::string hit = std::string(what[0].first, what[0].second);

		//Only get the expression inside the tag
		std::string tag = std::string(what[1].first, what[1].second);
		
		bool tag_matched = false;
		

		//Find out what type of tag this is
		boost::match_results<std::string::const_iterator> tagmatch;
		if( regex_match(tag, tagmatch, if_regex) ) //If tag
		{
			std::string expression = std::string(tagmatch[1].first, tagmatch[1].second);

			BOOST_LOG_TRIVIAL(trace) << "If tag with expression " << expression;

			tag_matched = true;
		}
		else if( regex_match(tag, tagmatch, for_regex) ) //For tag
		{
			std::string itemname = std::string(tagmatch[1].first, tagmatch[1].second); //The name that each element should take
			std::string loopname = std::string(tagmatch[2].first, tagmatch[2].second); //The name of the list of elements

			BOOST_LOG_TRIVIAL(trace) << "For tag. Iterate through " << loopname << " with each element named " << itemname;

			//Find the end of this for loop
			
			std::string::const_iterator new_start = what[0].second;
			int level = 0;

			boost::match_results<std::string::const_iterator> endfor_match;
			boost::match_results<std::string::const_iterator> fortag_match;

			bool fortag = false;
			bool endfor = false;

			bool found_loop_end = false;

			do
			{
				//Check to see for a tag
				fortag = regex_search(new_start, end, fortag_match, fortag_regex, flags);
				endfor = regex_search(new_start, end, endfor_match, endfortag_regex, flags);

				//Determine which tag appeared first
				if(fortag && endfor)
				{
					if(fortag_match[0].first < endfor_match[0].first)
					{
						endfor = false;
					}
					else
					{
						fortag = false;
					}
				}

				if(endfor)
				{
					if(level == 0)
					{
						std::string toreplace = std::string(what[0].first, endfor_match[0].second); //The segment that must be replaced
						std::string inlinecode = std::string(what[0].second, endfor_match[0].first); //The stuff inside the loop that must be repeated
				
						std::string replacement = "";
						NodeList* nodelist = (NodeList*)rootnode->get( loopname );

						//Expand the interior code for each node in the list
						foreach( Node *node, nodelist->nodes )
						{
							rootnode->properties[itemname] = node;
							replacement += expand(inlinecode, rootnode);
						}

						replace_all(result, toreplace, replacement); //Replace all instances of the loop with the new value

						found_loop_end = true;
						break;
					}
					else
					{
						level -= 1;
					}

					new_start =  endfor_match[0].second;
				}

				if(fortag)
				{
					level += 1;
					new_start =  fortag_match[0].second;
				}



			} while( fortag || endfor);

			if(found_loop_end == false)
			{
				BOOST_LOG_TRIVIAL(error) << "Coudn't find the end of this loop!";
			}

			tag_matched = true;
		}
		else if( regex_match(tag, tagmatch, variable_regex) ) //Standard variable tag
		{
			std::string variable = std::string(tagmatch[1].first, tagmatch[1].second); //The name of the variable
			std::string replacement = rootnode->get( variable )->toString(); //The text that the tag should be replaced with

			replace_all(result, hit, replacement); //Replace all instances of the tag with the new value

			BOOST_LOG_TRIVIAL(trace) << "Variable tag for variable " << variable;

			tag_matched = true;
		}

		if( tag_matched == true)
		{
			start = what[0].second;
		}
		else
		{
			start = what[0].first;
			++start;
		}

		
	}

	return result;
}

std::string templating::render(const char* filename, Node* rootnode)
{
	//Load file into string and then expand template
	std::string file = file_to_string(filename);
	std::string result = expand(file, rootnode);

	return result;
}