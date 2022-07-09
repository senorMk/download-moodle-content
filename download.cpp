#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <curl/curl.h>
#include "json.h"

#include <wx/filename.h>

std::string CleanString(const std::string& in)
{
	std::string c1 = in.substr(1, in.length());
	std::string c2 = c1.substr(0, c1.length() - 1);

	return c2;
}

size_t WriteData(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written;
}

size_t SendEventCurlWriteCallback(void* contents, size_t size, size_t nmemb, std::string* s)
{
	size_t newLength = size * nmemb;
	try
	{
		s->append((char*)contents, newLength);
	}
	catch (std::bad_alloc&)
	{
		return 0;
	}
	return newLength;
}

void CreateDirectoryRecursively(const std::wstring& directory) 
{
	try
	{
		static const std::wstring separators(L"\\/");

		// If the specified directory name doesn't exist, do our thing
		DWORD fileAttributes = ::GetFileAttributesW(directory.c_str());
		if (fileAttributes == INVALID_FILE_ATTRIBUTES) 
		{
			// Recursively do it all again for the parent directory, if any
			std::size_t slashIndex = directory.find_last_of(separators);
			if (slashIndex != std::wstring::npos) 
			{
				CreateDirectoryRecursively(directory.substr(0, slashIndex));
			}

			// Create the last directory on the path (the recursive calls will have taken
			// care of the parent directories by now)
			BOOL result = ::CreateDirectoryW(directory.c_str(), nullptr);
			if (result == FALSE) 
			{
				throw std::runtime_error("Could not create directory.");
			}
		}
		else 
		// Specified directory name already exists as a file or directory
		{
			bool isDirectoryOrJunction = ((fileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) ||
				((fileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0);

			if (!isDirectoryOrJunction) 
			{
				throw std::runtime_error( "Could not create directory because a file with the same name exists." );
			}
		}
	}
	catch (std::runtime_error ec) 
	{
		std::cout << "Error: Caught an runtime error of type: " << ec.what() << std::endl;
	}
}

int main()
{
	std::string moodleHome, username, password;
	std::cout << "Enter the Moodle homepage (ex : https://moodle.com):" << std::endl;
	std::cin >> moodleHome;
	std::cout << "Enter the Moodle account username:" << std::endl;
	std::cin >> username;
	std::cout << "Enter the Moodle account password:" << std::endl;
	std::cin >> password;

	try
	{
		// Get API token
		std::string userToken, userId;

		CURL* apiTokenCurl = curl_easy_init();
		if (apiTokenCurl)
		{
			std::string s;
			std::string url = moodleHome + std::string("/login/token.php?")
				+ std::string("username=") + username + std::string("&password=") + password
				+ std::string("&service=moodle_mobile_app");

			curl_easy_setopt(apiTokenCurl, CURLOPT_WRITEFUNCTION, SendEventCurlWriteCallback);
			curl_easy_setopt(apiTokenCurl, CURLOPT_WRITEDATA, &s);
			curl_easy_setopt(apiTokenCurl, CURLOPT_URL, url.data());
			curl_easy_setopt(apiTokenCurl, CURLOPT_VERBOSE, false);
			curl_easy_setopt(apiTokenCurl, CURLOPT_FOLLOWLOCATION, 0L);
			curl_easy_setopt(apiTokenCurl, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(apiTokenCurl, CURLOPT_SSL_VERIFYHOST, 0L);
			curl_easy_setopt(apiTokenCurl, CURLOPT_TIMEOUT, 10);

			struct curl_slist* tokenChunk = nullptr;
			tokenChunk = curl_slist_append(tokenChunk, "Expect:");
			curl_easy_setopt(apiTokenCurl, CURLOPT_HTTPHEADER, tokenChunk);

			curl_easy_perform(apiTokenCurl);

			// Parse the input
			json::jobject result = json::jobject::parse(s);

			// Get the token
			userToken = CleanString((std::string)result.get("token"));

			curl_easy_cleanup(apiTokenCurl);
			curl_slist_free_all(tokenChunk);
		}

		// Get the user ID
		 CURL* userIdCurl = curl_easy_init();
		if (userIdCurl)
		{
			std::string s;
			std::string url = moodleHome + std::string("/webservice/rest/server.php?moodlewsrestformat=json&wstoken=") +
				userToken + std::string("&wsfunction=core_webservice_get_site_info");

			curl_easy_setopt(userIdCurl, CURLOPT_WRITEFUNCTION, SendEventCurlWriteCallback);
			curl_easy_setopt(userIdCurl, CURLOPT_WRITEDATA, &s);
			curl_easy_setopt(userIdCurl, CURLOPT_URL, url.data());
			curl_easy_setopt(userIdCurl, CURLOPT_VERBOSE, false);
			curl_easy_setopt(userIdCurl, CURLOPT_FOLLOWLOCATION, 0L);
			curl_easy_setopt(userIdCurl, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(userIdCurl, CURLOPT_SSL_VERIFYHOST, 0L);
			curl_easy_setopt(userIdCurl, CURLOPT_TIMEOUT, 10);

			struct curl_slist* userIdChunk = NULL;
			userIdChunk = curl_slist_append(userIdChunk, "Expect:");
			curl_easy_setopt(userIdCurl, CURLOPT_HTTPHEADER, userIdChunk);

			curl_easy_perform(userIdCurl);

			// Parse the input
			json::jobject result = json::jobject::parse(s);

			// Get the user id
			userId = (std::string)result.get("userid");

			curl_easy_cleanup(userIdCurl);
			curl_slist_free_all(userIdChunk);
		}

		// Get the users' courses
		 CURL* userCoursesCurl = curl_easy_init();
		if (userCoursesCurl)
		{
			std::string s;
			std::string url = moodleHome + std::string("/webservice/rest/server.php?moodlewsrestformat=json&wstoken=") +
				userToken + std::string("&wsfunction=core_enrol_get_users_courses&userid=") + userId;

			curl_easy_setopt(userCoursesCurl, CURLOPT_WRITEFUNCTION, SendEventCurlWriteCallback);
			curl_easy_setopt(userCoursesCurl, CURLOPT_WRITEDATA, &s);
			curl_easy_setopt(userCoursesCurl, CURLOPT_URL, url.data());
			curl_easy_setopt(userCoursesCurl, CURLOPT_VERBOSE, false);
			curl_easy_setopt(userCoursesCurl, CURLOPT_FOLLOWLOCATION, 0L);
			curl_easy_setopt(userCoursesCurl, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(userCoursesCurl, CURLOPT_SSL_VERIFYHOST, 0L);
			curl_easy_setopt(userCoursesCurl, CURLOPT_TIMEOUT, 10);

			struct curl_slist* userCoursesChunk = nullptr;
			userCoursesChunk = curl_slist_append(userCoursesChunk, "Expect:");
			curl_easy_setopt(userCoursesCurl, CURLOPT_HTTPHEADER, userCoursesChunk);

			curl_easy_perform(userCoursesCurl);

			// Parse the input
			json::jobject result = json::jobject::parse(s);

			// For each course
			for (size_t i = 0; i < result.size(); i++)
			{
				json::jobject course = result.array(i);
				std::string courseId = course.get("id");

				wxString courseName = course.get("shortname");
				courseName.Replace("\"", "");
				char L2 = ' ';

				if (courseName.at(courseName.Length() - 1) == L2)
				{
					courseName.Remove(courseName.Length() - 1, 1);
				}

				// Get list of resources
				CURL* listResourcesCurl = curl_easy_init();
				if (listResourcesCurl)
				{
					std::string s;
					std::string url = moodleHome + std::string("/webservice/rest/server.php?moodlewsrestformat=json&wstoken=") +
						userToken + std::string("&courseids[0]=") + courseId +
						std::string("&wsfunction=mod_resource_get_resources_by_courses");

					curl_easy_setopt(listResourcesCurl, CURLOPT_WRITEFUNCTION, SendEventCurlWriteCallback);
					curl_easy_setopt(listResourcesCurl, CURLOPT_WRITEDATA, &s);
					curl_easy_setopt(listResourcesCurl, CURLOPT_URL, url.data());
					curl_easy_setopt(listResourcesCurl, CURLOPT_VERBOSE, false);
					curl_easy_setopt(listResourcesCurl, CURLOPT_FOLLOWLOCATION, 0L);
					curl_easy_setopt(listResourcesCurl, CURLOPT_SSL_VERIFYPEER, 0L);
					curl_easy_setopt(listResourcesCurl, CURLOPT_SSL_VERIFYHOST, 0L);
					curl_easy_setopt(listResourcesCurl, CURLOPT_TIMEOUT, 10);

					struct curl_slist* listResourcesChunk = NULL;
					listResourcesChunk = curl_slist_append(listResourcesChunk, "Expect:");
					curl_easy_setopt(listResourcesCurl, CURLOPT_HTTPHEADER, listResourcesChunk);

					curl_easy_perform(listResourcesCurl);

					// Cleanup
					curl_easy_cleanup(listResourcesCurl);
					curl_slist_free_all(listResourcesChunk);

					// Parse the input
					json::jobject result = json::jobject::parse(s);
					json::jobject resources = result.array(0);

					// For each resource
					size_t T = resources.size();
					for (size_t i = 0; i < T; i++)
					{
						std::string fileId = resources.array(i).get("coursemodule");

						std::string formedUrl = moodleHome + std::string("/mod/resource/view.php?id=") + fileId;

						json::jobject f = resources.array(i).get("contentfiles");

						json::jobject files = json::jobject::parse(f.array(0));

						std::string fileName = CleanString(files.get("filename"));
						std::string fileSize = CleanString(files.get("filesize"));

						wxString fileurl = CleanString(files.get("fileurl")) + std::string("?token=") + userToken;
						fileurl.Replace("\\/", "/");
						wxString mimeType = CleanString(files.get("mimetype"));
						mimeType.Replace("\\/", "/");

						// Download only the following file types: DOC, DOCX, PDF, PPT, PPTX
						if (mimeType == "application/msword" ||
							mimeType == "application/vnd.openxmlformats-officedocument.wordprocessingml.document" ||
							mimeType == "application/pdf" ||
							mimeType == "application/vnd.ms-powerpoint" ||
							mimeType == "application/vnd.openxmlformats-officedocument.presentationml.presentation")
						{
							// We set a hard limit of 10MB for each file
							const unsigned long LIMIT = 10 * 1048576;
							const unsigned long FILE_SIZE = std::stoull(fileSize);

							if (FILE_SIZE < LIMIT)
							{
								// Download it
								wxString W = courseName;
								CreateDirectoryRecursively(W.ToStdWstring());

								wxString filePath = W + "\\" + fileName;
								if (wxFileExists(filePath)) { wxRemoveFile(filePath); }

								std::cout << "Downloading file: " << filePath << std::endl;

								CURL* downloadFileCurl = curl_easy_init();
								if (downloadFileCurl)
								{
									 FILE* fp = fopen(filePath.ToStdString().c_str(), "wb");
									if (fp)
									{
										curl_easy_setopt(downloadFileCurl, CURLOPT_URL, fileurl.ToStdString().c_str());
										curl_easy_setopt(downloadFileCurl, CURLOPT_WRITEFUNCTION, WriteData);
										curl_easy_setopt(downloadFileCurl, CURLOPT_WRITEDATA, fp);
										curl_easy_setopt(downloadFileCurl, CURLOPT_FOLLOWLOCATION, 1);
										curl_easy_setopt(downloadFileCurl, CURLOPT_SSL_VERIFYPEER, FALSE);
										curl_easy_setopt(downloadFileCurl, CURLOPT_SSL_VERIFYHOST, FALSE);
										curl_easy_perform(downloadFileCurl);

										curl_easy_cleanup(downloadFileCurl);
										fclose(fp);
									}
									else
									{
										std::cout << "Error: Could not download file: " << filePath << std::endl;
									}
								}
							}
						}
					}
				}
			}

			curl_easy_cleanup(userCoursesCurl);
			curl_slist_free_all(userCoursesChunk);
		}
	}
	catch (std::exception& except)
	{
		std::cout << "Error: Caught an exception of type: " << except.what() << std::endl;
	}

	std::cout << "Downloads complete." << std::endl;
}