class TumblrDl < Formula
  desc "Download Tumblr videos from the command-line."
  homepage "https://github.com/dx7/tumblr-dl"
  url "https://github.com/dx7/tumblr-dl/archive/v0.3.tar.gz"
  sha256 "6103bafe059383e7ad3a51cfc82c4cc096594b640b4c1bcdb00ba012c06ebc26"

  depends_on "curl"

  def install
    system "./configure", "--prefix=#{prefix}"
    system "make", "install"
  end

  test do
    system "#{bin}/tumblr-dl", "http://funnyfailvideos.tumblr.com/post/136192693616/funny-video-23"
  end
end
