;;; mtype-mode.el --- Major mode for mType programming language with LSP support

;;; Commentary:
;; This package provides a major mode for the mType programming language
;; with Language Server Protocol support via lsp-mode.

;;; Code:

(require 'lsp-mode)

;; Define mType major mode
(define-derived-mode mtype-mode prog-mode "mType"
  "Major mode for editing mType source code."
  :syntax-table mtype-mode-syntax-table

  ;; Comments
  (setq-local comment-start "// ")
  (setq-local comment-end "")
  (setq-local comment-start-skip "//+\\s-*")

  ;; Indentation
  (setq-local indent-line-function 'mtype-indent-line)
  (setq-local tab-width 4)

  ;; Keywords
  (setq-local font-lock-defaults '(mtype-font-lock-keywords)))

;; Syntax table
(defvar mtype-mode-syntax-table
  (let ((table (make-syntax-table)))
    ;; C++ style comments
    (modify-syntax-entry ?/ ". 124b" table)
    (modify-syntax-entry ?* ". 23" table)
    (modify-syntax-entry ?\n "> b" table)
    ;; Strings
    (modify-syntax-entry ?\" "\"" table)
    (modify-syntax-entry ?\' "\"" table)
    ;; Operators
    (modify-syntax-entry ?+ "." table)
    (modify-syntax-entry ?- "." table)
    (modify-syntax-entry ?* "." table)
    (modify-syntax-entry ?/ "." table)
    (modify-syntax-entry ?% "." table)
    (modify-syntax-entry ?< "." table)
    (modify-syntax-entry ?> "." table)
    (modify-syntax-entry ?& "." table)
    (modify-syntax-entry ?| "." table)
    (modify-syntax-entry ?= "." table)
    table)
  "Syntax table for `mtype-mode'.")

;; Font lock keywords
(defconst mtype-font-lock-keywords
  `(
    ;; Keywords
    (,(regexp-opt '("class" "interface" "function" "if" "else" "while" "for" "foreach"
                    "return" "new" "this" "super" "extends" "implements"
                    "public" "private" "protected" "static" "final" "abstract"
                    "async" "await" "try" "catch" "finally" "throw"
                    "break" "continue" "switch" "case" "default" "do"
                    "import" "from") 'words)
     . font-lock-keyword-face)

    ;; Types
    (,(regexp-opt '("int" "float" "string" "bool" "void"
                    "List" "HashMap" "HashSet" "Stack" "Queue" "LinkedList"
                    "Promise") 'words)
     . font-lock-type-face)

    ;; Constants
    (,(regexp-opt '("true" "false" "null") 'words)
     . font-lock-constant-face)

    ;; Function names
    ("\\<\\(function\\)\\s-+\\([a-zA-Z_][a-zA-Z0-9_]*\\)"
     (2 font-lock-function-name-face))

    ;; Class names
    ("\\<\\(class\\|interface\\)\\s-+\\([a-zA-Z_][a-zA-Z0-9_]*\\)"
     (2 font-lock-type-face))

    ;; Numbers
    ("\\<[0-9]+\\(\\.[0-9]+\\)?\\>" . font-lock-constant-face)

    ;; Strings
    ("\"[^\"]*\"" . font-lock-string-face))
  "Keyword highlighting for mType mode.")

;; Simple indentation
(defun mtype-indent-line ()
  "Indent current line as mType code."
  (interactive)
  (let ((indent-col 0))
    (save-excursion
      (beginning-of-line)
      (condition-case nil
          (progn
            (backward-up-list)
            (setq indent-col (+ (current-indentation) tab-width)))
        (error (setq indent-col 0))))
    (indent-line-to indent-col)))

;; Register file extension
(add-to-list 'auto-mode-alist '("\\.mt\\'" . mtype-mode))

;; LSP integration
(add-to-list 'lsp-language-id-configuration '(mtype-mode . "mtype"))

(lsp-register-client
 (make-lsp-client
  :new-connection (lsp-stdio-connection '("mtype-language-server" "--stdio"))
  :major-modes '(mtype-mode)
  :server-id 'mtype
  :priority 0))

;; Automatically enable LSP in mtype-mode
(add-hook 'mtype-mode-hook #'lsp-deferred)

(provide 'mtype-mode)
;;; mtype-mode.el ends here
