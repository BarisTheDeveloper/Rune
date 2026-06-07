#ifndef RUNE_UI_WINDOW_HPP
#define RUNE_UI_WINDOW_HPP

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <string>
#include <functional>

namespace rune::ui {

class Window {
public:
  Window();
  ~Window();

  // Create window with GTK + WebKitGTK
  bool create(int width, int height, const std::string& title);

  // Load a URL or local file
  void loadUrl(const std::string& url);
  void loadHtml(const std::string& html);

  // Run GTK main loop (blocks until window closes)
  void run();

  // Set callback for IPC messages from JS bridge
  using BridgeCallback = std::function<std::string(const std::string& json)>;
  void onBridgeMessage(BridgeCallback cb);

  // Send result back to JS
  void sendToJs(const std::string& jsCall);

  bool isOpen() const { return m_window != nullptr; }

private:
  GtkWidget* m_window;
  WebKitWebView* m_webview;
  BridgeCallback m_bridgeCallback;

  static void onDestroy(GtkWidget*, gpointer);
  static void onWebViewReady(WebKitWebView*, GParamSpec*, gpointer);
  static void onScriptMessage(WebKitUserContentManager*,
                              WebKitJavascriptResult*,
                              gpointer);
};

} // namespace rune::ui

#endif
