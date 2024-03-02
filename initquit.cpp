
namespace {

class myinitquit : public initquit {
  public:
	void on_init() {}
	void on_quit() {}
};

FB2K_SERVICE_FACTORY( myinitquit );

}  // namespace
