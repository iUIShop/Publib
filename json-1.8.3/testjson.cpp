//#include "value.h"
//#include "reader.h"
#include <string>
#include <exception>
#include <stddef.h>
#include "json/json.h"

int main(int argc,char* argv[])
{
	const char* json_str = "{"
					"\"values\" : [ \"SSH-7408\","
									"\"BOOT-5122\""
							"]}";

	Json::Reader _reader;

	Json::Value null (Json::nullValue);
	Json::Value jvRoot;
	jvRoot.swap(null);

	bool ok = _reader.parse(json_str, jvRoot);
	Json::Value::iterator it;
	ok = jvRoot["values"].isArray();
	if(ok) {
		Json::Value::iterator it;
		// for(it = jvRoot["values"].begin();it != jvRoot["values"].end();it++) {
		// 	const char* pstr = (*it).asCString();
		// 	std::string value = pstr;
		// 	std::cout << value << std::endl;
		// }

        for(int i = 0;i < jvRoot["values"].size();i++) {
            std::cout << jvRoot["values"][i].asCString() << std::endl;
        }
	}


	return 0;
}
