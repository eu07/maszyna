#ifdef WITH_CRASHPAD
void crashreport_add_info(const char *name, const std::string &value);
const std::string& crashreport_get_provider();
void crashreport_set_autoupload();
bool crashreport_is_pending();
void crashreport_upload_reject();
void crashreport_upload_accept();
#else
#define crashreport_add_info(a,b)
#define crashreport_get_provider() (std::string(""))
#define crashreport_set_autoupload()
#define crashreport_is_pending() (false)
#define crashreport_upload_reject()
#define crashreport_upload_accept()
#endif
