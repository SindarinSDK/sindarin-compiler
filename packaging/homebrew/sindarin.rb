# Homebrew formula for Sindarin
# Install with: brew install --formula ./sindarin.rb

class Sindarin < Formula
  desc "Statically-typed procedural language that compiles to C"
  homepage "https://github.com/SindarinSDK/sindarin-compiler"
  url "https://github.com/SindarinSDK/sindarin-compiler/releases/download/${TAG}/sindarin-${VERSION}-macos-x64.tar.gz"
  sha256 "${SHA256}"
  license "MIT"
  version "${VERSION}"

  depends_on xcode: :build

  def install
    # Install entire SDK root to lib/sindarin/
    (lib/"sindarin").install Dir["lib/sindarin/*"]
    # Symlink the binary into bin/
    bin.install_symlink lib/"sindarin/sn"
  end

  def caveats
    <<~EOS
      Sindarin requires a C compiler (clang) which is provided by Xcode Command Line Tools.
      If not already installed, run:
        xcode-select --install

      To compile a Sindarin program:
        sn hello.sn -o hello
        ./hello
    EOS
  end

  test do
    (testpath/"hello.sn").write <<~EOS
      fn main(): void =>
        print("Hello, Homebrew!\\n")
    EOS
    system "#{bin}/sn", "hello.sn", "-o", "hello"
    assert_equal "Hello, Homebrew!\n", shell_output("./hello")
  end
end
