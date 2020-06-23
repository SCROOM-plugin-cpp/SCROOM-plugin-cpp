#include <boost/dll.hpp>
#include <boost/test/unit_test.hpp>

// Make all private members accessible for testing
#define private public

#include "../src/pipette.hh"

BOOST_AUTO_TEST_SUITE(Pipette_Tests)

class DummyPresentation : public PresentationInterface, public PipetteViewInterface {
	Scroom::Utils::Rectangle<double> getRect() { return Scroom::Utils::Rectangle<int>(0, 0, 0, 0); }
	void redraw(ViewInterface::Ptr const&, cairo_t*, Scroom::Utils::Rectangle<double>, int) {}
	bool getProperty(const std::string&, std::string&) { return false; }
	bool isPropertyDefined(const std::string&) { return false; }
	std::string getTitle() { return nullptr; }
	void open(ViewInterface::WeakPtr) {};
	void close(ViewInterface::WeakPtr) {};
	PipetteLayerOperations::PipetteColor getPixelAverages(Scroom::Utils::Rectangle<int>) {
        return {{"C", 1.0}};
	}
};

static int reg_sel = 0;
static int reg_post = 0;
static int tool_btn = 0;
static PresentationInterface::Ptr presentation;
static int msg_set = 0;

class DummyView : public ViewInterface {
	void invalidate() {}
	ProgressInterface::Ptr getProgressInterface() { return nullptr; }
	void addSideWidget(std::string, GtkWidget*) {}
	void removeSideWidget(GtkWidget*) {}
	void addToToolbar(GtkToolItem*) {}
	void removeFromToolbar(GtkToolItem*) {}
	void registerSelectionListener(SelectionListener::Ptr) { reg_sel++; }
	void registerPostRenderer(PostRenderer::Ptr) { reg_post++; }
	void setStatusMessage(const std::string& msg) {
		msg_set++;
		//known text
		if (msg.compare("Computing color values...") || msg.compare("Pipette is not supported for this presentation.")) {
			return;
		}

		//actually part of the value_display test case
		if (msg.compare("Top-left: 10, Bottom-right: 11, Height: 12, Width: 13, Colors: C: 1.00")) {
			return;
		}

		BOOST_CHECK(false);
	}
	PresentationInterface::Ptr getCurrentPresentation() { return presentation; }
	void addToolButton(GtkToggleButton*, ToolStateListener::Ptr) { tool_btn++; }
};

static int view_observers = 0;

class DummyPluginInterface : public ScroomPluginInterface{
  void registerNewPresentationInterface(const std::string&, NewPresentationInterface::Ptr) {};
  void registerNewAggregateInterface(const std::string&, NewAggregateInterface::Ptr) {};
  void registerOpenPresentationInterface(const std::string&, OpenPresentationInterface::Ptr) {};
  void registerOpenInterface(const std::string&, OpenInterface::Ptr) {};
  void registerViewObserver(const std::string&, ViewObserver::Ptr) { view_observers++; };
  void registerPresentationObserver(const std::string&, PresentationObserver::Ptr) {};
};

BOOST_AUTO_TEST_CASE(metadata) {
	Pipette::Ptr pipette = Pipette::create();

	int pre_view_observers = view_observers;

	pipette->registerCapabilities(ScroomPluginInterface::Ptr(new DummyPluginInterface()));

	//maybe not worth testing
	BOOST_CHECK(pipette->getPluginName() == "Pipette");
	BOOST_CHECK(!pipette->getPluginVersion().empty());
    BOOST_CHECK(pre_view_observers + 1 == view_observers);
}

BOOST_AUTO_TEST_CASE(value_display) {
	PipetteHandler::Ptr handler = PipetteHandler::create();

	presentation = PresentationInterface::Ptr(new DummyPresentation());

	int pre_msg_set = msg_set;

	//test will fail via event handlers
    handler->computeValues(ViewInterface::Ptr(new DummyView), Scroom::Utils::Rectangle<int>(10, 11, 12, 13));

    BOOST_CHECK(pre_msg_set + 1 == msg_set);
}

BOOST_AUTO_TEST_CASE(view_add) {
	Pipette::Ptr pipette = Pipette::create();

	int pre_reg_sel = reg_sel;
	int pre_reg_post = reg_post;
    int pre_tool_btn = tool_btn;

    Scroom::Bookkeeping::Token token = pipette->viewAdded(ViewInterface::Ptr(new DummyView()));

    BOOST_CHECK(pre_reg_sel + 1 == reg_sel);
    BOOST_CHECK(pre_reg_post + 1 == reg_post);
    BOOST_CHECK(pre_tool_btn + 1 == tool_btn);
    BOOST_CHECK(token != nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
