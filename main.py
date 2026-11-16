import sys
import os
import subprocess
import re
import shutil
from time import time

def benchmark(func):
  def timer(*args, **kwargs):
    start = time()
    result = func(*args, **kwargs)
    stop = time()
    print('Time taken : %.3fs' %(stop - start))
    return result
  return timer

section = ['section', 'subsection', 'subsubsection', 'paragraph']
text = ''

minted_opts = """
    breaklines=true,
    frame=lines,
    fontfamily=tt,
    linenos=false,
    numberblanklines=true,
    numbersep=2pt,
    gobble=0,
    framerule=1pt,
    framesep=1mm,
    funcnamehighlighting=true,
    tabsize=2,
    obeytabs=false,
    mathescape=false,
    samepage=false,
    showspaces=false,
    showtabs=false,
    texcl=false
"""

EXT_TO_LANG = {
    '.cpp': 'C++',
    '.cc': 'C++',
    '.cxx': 'C++',
    '.h': 'C++',
    '.hpp': 'C++',
    '.py': 'python',
    '.java': 'java',
    '.sh': 'bash',
}

COMMENT_TYPES = [
    ('/**', '*/'),
    ("'''", "'''"),
    ('"""', '"""'),
]

def find_start_comment(source, start=None):
    first = (-1, -1, None)
    for s, e in COMMENT_TYPES:
        i = source.find(s, start)
        if i != -1 and (i < first[0] or first[0] == -1):
            first = (i, i + len(s), e)

    return first

def parse_include(line):
    line = line.strip()
    if line.startswith("#include"):
        return line[8:].strip()
    return None

def escape(input_str):
    input_str = input_str.replace('<', r'\ensuremath{<}')
    input_str = input_str.replace('>', r'\ensuremath{>}')
    return input_str

def pathescape(input_str):
    input_str = input_str.replace('\\', r'\\')
    input_str = input_str.replace('_', r'\_')
    input_str = escape(input_str)
    return input_str

def codeescape(input_str):
    input_str = input_str.replace('_', r'\_')
    input_str = input_str.replace('\n', '\\\\\n')
    input_str = input_str.replace('{', r'\{')
    input_str = input_str.replace('}', r'\}')
    input_str = input_str.replace('^', r'\ensuremath{\hat{\;}}')
    input_str = escape(input_str)
    return input_str

def ordoescape(input_str, esc=True):
    if esc:
        input_str = escape(input_str)
    start = input_str.find("O(")
    if start >= 0:
        bracketcount = 1
        end = start + 1
        while end + 1 < len(input_str) and bracketcount > 0:
            end = end + 1
            if input_str[end] == '(':
                bracketcount = bracketcount + 1
            elif input_str[end] == ')':
                bracketcount = bracketcount - 1
        if bracketcount == 0:
            return r"%s\bigo{%s}%s" % (input_str[:start], input_str[start+2:end], ordoescape(input_str[end+1:], False))
    return input_str

def processwithcomments(caption):
  code = ''
  ext = os.path.splitext(caption)[1].lower()
  lang = EXT_TO_LANG.get(ext, 'C++')

  knowncommands = ['Author', 'Date', 'Description', 'Source', 'Time', 'Memory', 'License', 'Status', 'Usage', 'Details']
  requiredcommands = []
  includelist = []
  error = ""
  warning = ""

  # Read lines from source file safely
  try:
    with open(caption, 'r', encoding='utf-8', errors='replace') as instream:
      lines = instream.readlines()
  except Exception as e:
    error = f"Could not read source: {e}"
    lines = []

  nlines = list()
  for line in lines:
    if 'exclude-line' in line:
      continue
    if 'include-line' in line:
      line = line.replace('// ', '', 1)
    had_comment = "///" in line
    keep_include = 'keep-include' in line
    # Remove /// comments
    line = line.split("///")[0].rstrip()
    # Remove '#pragma once' lines
    if line == "#pragma once":
      continue
    if had_comment and not line:
      continue
    nlines.append(line)

  # Remove and process multiline comments
  source = '\n'.join(nlines)
  nsource = ''
  start, start2, end_str = find_start_comment(source)
  end = 0
  commands = {}
  while start >= 0 and not error:
    nsource = nsource.rstrip() + source[end:start]
    end = source.find(end_str, start2)
    if end < start:
      error = "Invalid %s %s comments." % (source[start:start2], end_str)
      break
    comment = source[start2:end].strip()
    end += len(end_str)
    start, start2, end_str = find_start_comment(source, end)

    commentlines = comment.split('\n')
    command = None
    value = ""
    for cline in commentlines:
      allow_command = False
      cline = cline.strip()
      if cline.startswith('*'):
        cline = cline[1:].strip()
        allow_command = True
      elif end_str in ["'''", '"""']:
        allow_command = True

      ind = cline.find(':')
      if allow_command and ind != -1 and ' ' not in cline[:ind] and len(cline) > 0 and cline[0].isalpha() and cline[0].isupper():
        if command:
          if command not in knowncommands:
            error = error + "Unknown command: " + command + ". "
          commands[command] = value.lstrip()
        command = cline[:ind]
        value = cline[ind+1:].strip()
      else:
        value = value + '\n' + cline
    if command:
      if command not in knowncommands:
        error = error + "Unknown command: " + command + ". "
      commands[command] = value.lstrip()

  for rcommand in sorted(set(requiredcommands) - set(commands)):
    error = error + "Missing command: " + rcommand + ". "
  if end >= 0:
    nsource = nsource.rstrip() + source[end:]
  nsource = nsource.strip()

  # Produce output
  out = []
  if warning:
    print(r"warning{%s: %s}" % (caption, warning))
  if error:
    print(r"error{%s: %s}" % (caption, error))
  else:
    if commands.get("Description"):
        out.append(r"\printdescription{%s}" % escape(commands["Description"]))
    if commands.get("Usage"):
        out.append(r"\printusage{%s}" % codeescape(commands["Usage"]))
    if commands.get("Time"):
        out.append(r"\printtime{%s}" % ordoescape(commands["Time"]))
    if commands.get("Memory"):
        out.append(r"\printmemory{%s}" % ordoescape(commands["Memory"]))
    out.append(f'\\begin{{minted}}[{minted_opts}]{{{lang}}}\n')
    out.append(nsource)
    out.append('\\end{minted}\n')

  for line in out:
    code = code + line + '\n'
 
  return code

def gen(level):
  global text

  folders = [x for x in os.listdir(".") if os.path.isdir(x) and x != ".git"]
  files = [x for x in os.listdir(".") if os.path.isfile(x) and x != ".gitignore"]
  
  folders.sort()
  if 'Setup' in folders:
    folders.remove('Setup')
    folders.insert(0, 'Setup')
  files.sort()

  parent = os.getcwd()
  sec_tag = section[min(level, len(section) - 1)]

  for folder in folders:
    escaped_folder = folder.replace('_', r'\_')
    text = text + '\\' + sec_tag + '{' + escaped_folder + '}\n'
    child = os.path.join(parent, folder)
    os.chdir(child)
    gen(level + 1)
    os.chdir(parent)

  for file in files:
    name, ext = os.path.splitext(file)
    if ext not in ['.cpp', '.py', '.java', '.tex', '.h']:
      continue
    escaped_name = name.replace('_', r'\_')

    text = text + '\\' + sec_tag + '{' + escaped_name + '}\n'
    if ext == '.tex':
      try:
        with open(file, 'r', encoding='utf-8', errors='replace') as tex_f:
          tex_content = tex_f.read()
        # Remove \end{document} if present so the outer document remains open
        tex_content = re.sub(r'\\end\{document\}', '', tex_content)
        text = text + tex_content + '\n'
      except Exception as e:
        print(f"Error reading {file}: {e}")
    else:
      source = processwithcomments(file)
      text = text + source

@benchmark
def main():
  global text

  main_dir = os.getcwd()

  if not os.path.isdir('code'):
    print("Error!\nFolder 'code' not found!")
    sys.exit(1)

  os.chdir('code')
  gen(0)
  os.chdir(main_dir)

  if not os.path.isdir('template'):
    print("Error!\nFolder 'template' not found!")
    sys.exit(1)

  text = text + '\\end{multicols*}\n'
  text = text + '\\end{document}\n'

  template_dir = os.path.join(main_dir, 'template')
  os.chdir(template_dir)
  try:
    with open('template.tex', 'r', encoding='utf-8') as f:
      template_content = f.read()
    
    full_text = template_content + text
    with open('notebook.tex', 'w', encoding='utf-8') as f:
      f.write(full_text)

    print("Generating Notebook...")
    print("This may take a while. Please wait...")
    for i in range(4):
      subprocess.run(
        'pdflatex -interaction=nonstopmode --shell-escape notebook.tex',
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
      )

    if os.path.exists('_minted-notebook'):
      shutil.rmtree('_minted-notebook', ignore_errors=True)

    if os.path.exists('notebook.pdf'):
      target_pdf = os.path.join(main_dir, 'notebook.pdf')
      shutil.move('notebook.pdf', target_pdf)
      # Clean up intermediate notebook compilation files
      for f in os.listdir('.'):
        if f.startswith('notebook.') and f != 'notebook.tex':
          try:
            os.remove(f)
          except OSError:
            pass
      if os.path.exists('notebook.tex'):
        os.remove('notebook.tex')
      print('Done!')
    else:
      print("Error: notebook.pdf was not generated. Please check template/notebook.log.")
  finally:
    os.chdir(main_dir)

if __name__ == '__main__':
  main()
