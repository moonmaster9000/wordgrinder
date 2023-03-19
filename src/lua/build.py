from build.ab2 import normalrule
from build.c import clibrary
from tools.build import multibin

SRCS = [
    "src/lua/_prologue.lua",
    "src/lua/events.lua",
    "src/lua/main.lua",
    "src/lua/xml.lua",
    "src/lua/utils.lua",
    "src/lua/redraw.lua",
    "src/lua/settings.lua",
    "src/lua/document.lua",
    "src/lua/forms.lua",
    "src/lua/ui.lua",
    "src/lua/browser.lua",
    "src/lua/html.lua",
    "src/lua/margin.lua",
    "src/lua/xpattern.lua",
    "src/lua/fileio.lua",
    "src/lua/export.lua",
    "src/lua/export/text.lua",
    "src/lua/export/html.lua",
    "src/lua/export/latex.lua",
    "src/lua/export/troff.lua",
    "src/lua/export/opendocument.lua",
    "src/lua/export/org.lua",
    "src/lua/export/markdown.lua",
    "src/lua/import.lua",
    "src/lua/import/html.lua",
    "src/lua/import/text.lua",
    "src/lua/import/opendocument.lua",
    "src/lua/import/markdown.lua",
    "src/lua/navigate.lua",
    "src/lua/addons/goto.lua",
    "src/lua/addons/autosave.lua",
    "src/lua/addons/docsetman.lua",
    "src/lua/addons/gui.lua",
    "src/lua/addons/scrapbook.lua",
    "src/lua/addons/statusbar_charstyle.lua",
    "src/lua/addons/statusbar_pagecount.lua",
    "src/lua/addons/statusbar_position.lua",
    "src/lua/addons/statusbar_wordcount.lua",
    "src/lua/addons/debug.lua",
    "src/lua/addons/look-and-feel.lua",
    "src/lua/addons/keymapoverride.lua",
    "src/lua/addons/smartquotes.lua",
    "src/lua/addons/undo.lua",
    "src/lua/addons/spillchocker.lua",
    "src/lua/addons/templates.lua",
    "src/lua/addons/directories.lua",
    "src/lua/addons/recents.lua",
    "src/lua/colours.lua",
    "src/lua/menu.lua",
    "src/lua/cli.lua",
]

multibin(
    name="luacode",
    symbol="script_table",
    srcs=SRCS,
)

normalrule(
    name="typecheck",
    ins=["tools+typechecker", "./_types.def"] + SRCS,
    outs=["stamp"],
    label="TYPECHECK",
    commands=["{ins[0]} -t {ins[1]} " + " ".join(SRCS)],
)
