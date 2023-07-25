import sys
import importlib
import argparse

def html_header(module_name):
    print(
"""
<!DOCTYPE html>
<html lang="en">
    <head>
        <meta charset="utf-8"/>
        <title>Python API Documentation: %s</title>
        <link rel="stylesheet" href="coalpy_docs_style.css"/>
        <script>
                document.addEventListener('DOMContentLoaded', function()
                {
                    g_sidebar = document.getElementsByClassName("sidebar")[0];
                    g_sidebar_ul = g_sidebar.getElementsByTagName("ul")[0];
                    g_sidebar_lis = g_sidebar_ul.getElementsByTagName("li")
                    for (var i = 0; i < g_sidebar_lis.length; ++i)
                    {
                        var licontent = g_sidebar_lis[i].getElementsByClassName("treelink")[0];
                        if (licontent == null)
                            continue;
                        
                        var licontentDot = licontent.getElementsByClassName("treedot")[0];
                        licontent.parentLi = g_sidebar_lis[i];
                        licontent.dot = licontentDot == undefined ? null : licontentDot;
                        licontent.addEventListener("click",
                        function()
                        {
                            var children = this.parentLi.getElementsByTagName("ul")[0];
                            if (children == undefined || children == null)
                                return;

                            if (children.style.display == 'none')
                            {
                                if (this.dot != null) this.dot.innerHTML = "+";
                                children.style.display = 'block';
                            }
                            else
                            {
                                if (this.dot != null) this.dot.innerHTML = "-";
                                children.style.display = 'none';
                            }
                        });
                    }
                });
        </script>
    </head>
""" % (module_name), end="")

def html_doc_bar_item_name(name, can_expand=True):
    return """<div class="treelink">%s<span>%s</span></div>""" % ('<div class="treedot">+</div>' if can_expand else "", name)

def html_doc_bar(mod):
    print("""
        <div class="sidebar">""", end="")

    allsymbols = dir(mod)
    publicsymbols = [(nm, getattr(mod, nm)) for nm in allsymbols if not (nm.startswith("__") or nm.startswith("Enum"))]
    funcsymbols = [(nm, s) for (nm, s) in publicsymbols if callable(s) and type(s) != type]
    typesymbols = [(nm, s) for (nm, s) in publicsymbols if (type(s) == type and not type(s).__name__.startswith("Enum"))]
    enumsymbols = [(nm, s) for (nm, s) in publicsymbols if (type(s).__name__.startswith("Enum"))]
    print("""
            <ul><li>""", end="")
    print(html_doc_bar_item_name("Functions"), end="")
    print("""
                <ul>""", end="")
    for (nm, s) in funcsymbols:
        print("""
                    <li>""", end="")
        print(html_doc_bar_item_name(nm, False), end="")
        print("""
                    </li>""", end="")
    print("""
                </ul>""", end="")

    print("""
            </li>""", end="")

    print("""
            <li>""", end="")
    print(html_doc_bar_item_name("Types"), end="")
    print("""
                <ul>""", end="")
    for (nm, s) in typesymbols:
        print("""
                    <li>""", end="")
        print(html_doc_bar_item_name(nm, False), end="")
        print("""
                    </li>""", end="")
    print("""
                </ul>""", end="")
    print("""
            </li>""", end="")

    print("""
            <li>""", end="")
    print(html_doc_bar_item_name("Enums"), end="")
    print("""
                <ul>""", end="")
    for (nm, s) in enumsymbols:
        print("""
                    <li>""", end="")
        print(html_doc_bar_item_name(nm, False), end="")
        print("""
                    </li>""", end="")
    print("""
                </ul>""", end="")
    print("""
            </li>""", end="")

    print("""
            </ul>""", end="")
    print("""
        </div>""", end="")



def html_body(mod):
    print("""
    <body>""", end="")
    html_doc_bar(mod)
    print("""
    </body>""", end="")

def html_footer():
    print("""
</html>""")

def gen_doc(module_name):
    mod = importlib.import_module(module_name)
    html_header(module_name)
    html_body(mod)
    html_footer()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog="python doctool.py",
        description = "Simple hackable tool to generate documentation in html")
    parser.add_argument("module_name")

    args = parser.parse_args()
    gen_doc(args.module_name)
