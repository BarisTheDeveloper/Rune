#include "ui/window.hpp"
#include <webkit2/webkit2.h>
#include <jsc/jsc.h>
#include <iostream>

namespace rune::ui {

Window::Window() : m_window(nullptr), m_webview(nullptr) {}

Window::~Window() {
  if (m_window) {
    gtk_widget_destroy(m_window);
  }
}

void Window::onDestroy(GtkWidget*, gpointer) {
  std::cout << "[RUNE] Window closed." << std::endl;
  gtk_main_quit();
}

void Window::onWebViewReady(WebKitWebView*, GParamSpec*, gpointer userdata) {
  auto* win = static_cast<Window*>(userdata);
  std::cout << "[RUNE] WebView loaded." << std::endl;

  // Inject the Rune bridge JavaScript
  const char* bridgeJs = R"JS(
    (function() {
      if (window.rune) return;

      window.rune = {
        _id: 0,
        _pending: {},

        call: function(method, params) {
          params = params || {};
          var id = ++this._id;
          var self = this;
          return new Promise(function(resolve, reject) {
            self._pending[id] = { resolve: resolve, reject: reject };
            window.webkit.messageHandlers.rune.postMessage(
              JSON.stringify({ id: id, method: method, params: params })
            );
          });
        },

        _resolve: function(id, error, result) {
          var p = this._pending[id];
          if (!p) return;
          delete this._pending[id];
          if (error) {
            p.reject(new Error(error));
          } else {
            p.resolve(result);
          }
        }
      };

      console.log('[Rune] Bridge ready');
    })();
  )JS";

  WebKitUserContentManager* mgr =
    webkit_web_view_get_user_content_manager(win->m_webview);
  webkit_user_content_manager_add_script(
    mgr,
    webkit_user_script_new(
      bridgeJs,
      WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,
      WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
      nullptr,
      nullptr
    )
  );
}

void Window::onScriptMessage(WebKitUserContentManager*,
                             WebKitJavascriptResult* result,
                             gpointer userdata) {
  auto* win = static_cast<Window*>(userdata);

  JSCValue* jsValue = webkit_javascript_result_get_js_value(result);
  gchar* strValue = jsc_value_to_string(jsValue);
  std::string msg(strValue ? strValue : "{}");
  g_free(strValue);

  // Forward to bridge callback if set
  if (win->m_bridgeCallback) {
    std::string response = win->m_bridgeCallback(msg);
    if (!response.empty()) {
      win->sendToJs(response);
    }
  }
}

bool Window::create(int width, int height, const std::string& title) {
  m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(m_window), title.c_str());
  gtk_window_set_default_size(GTK_WINDOW(m_window), width, height);
  g_signal_connect(m_window, "destroy", G_CALLBACK(onDestroy), this);

  // Create WebView
  m_webview = WEBKIT_WEB_VIEW(webkit_web_view_new());
  g_signal_connect(m_webview, "notify::title",
                   G_CALLBACK(onWebViewReady), this);

  // Register script message handler for Rune bridge
  WebKitUserContentManager* mgr =
    webkit_web_view_get_user_content_manager(m_webview);
  g_signal_connect(mgr, "script-message-received::rune",
                   G_CALLBACK(onScriptMessage), this);
  webkit_user_content_manager_register_script_message_handler(mgr, "rune");

  // Enable DevTools
  WebKitSettings* settings = webkit_web_view_get_settings(m_webview);
  webkit_settings_set_enable_developer_extras(settings, TRUE);

  gtk_container_add(GTK_CONTAINER(m_window), GTK_WIDGET(m_webview));
  gtk_widget_show_all(m_window);

  std::cout << "[RUNE] Window created: " << width << "x" << height << std::endl;
  return true;
}

void Window::loadUrl(const std::string& url) {
  if (m_webview) {
    webkit_web_view_load_uri(m_webview, url.c_str());
  }
}

void Window::loadHtml(const std::string& html) {
  if (m_webview) {
    webkit_web_view_load_html(m_webview, html.c_str(), nullptr);
  }
}

void Window::run() {
  gtk_main();
}

void Window::onBridgeMessage(BridgeCallback cb) {
  m_bridgeCallback = cb;
}

void Window::sendToJs(const std::string& jsCall) {
  // Call JS function in webview context
  // jsCall should be like: "rune._resolve(1, null, 'hello')"
  std::string escaped = jsCall;
  // Escape quotes for JS string
  size_t pos = 0;
  while ((pos = escaped.find('\'', pos)) != std::string::npos) {
    escaped.replace(pos, 1, "\\'");
    pos += 2;
  }

  std::string code = escaped + ";";
  webkit_web_view_evaluate_javascript(m_webview, code.c_str(), -1, nullptr, nullptr, nullptr, nullptr, nullptr);
}

} // namespace rune::ui
