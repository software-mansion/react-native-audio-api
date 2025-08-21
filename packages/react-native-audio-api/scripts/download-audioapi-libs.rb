require "net/http"
require "uri"
require "fileutils"

def download_static_libraries(config)
    platforms = ['iphoneos', 'iphonesimulator']
    
    platforms.each do |platform|
      config[:lib_names].each do |lib|
        lib_url = "#{config[:base_url]}/ios/#{platform}/lib#{lib}.a"
        target_file = File.join(config[:target_dir], platform, "lib#{lib}.a")
        
        FileUtils.mkdir_p(File.dirname(target_file))
        
        unless File.exist?(target_file)
          
          begin
            uri = URI(lib_url)
            Net::HTTP.start(uri.host, uri.port, use_ssl: uri.scheme == 'https') do |http|
              request = Net::HTTP::Get.new(uri)
              response = http.request(request)
              
              if response.code == '200'
                File.open(target_file, 'wb') do |file|
                  file.write(response.body)
                end
              end
            end
          end
        end
      end
    end
  end
  
  def download_include_folder(config)
    include_url = "#{config[:base_url]}/include.zip"
    target_include_dir = config[:target_dir]

    FileUtils.mkdir_p(target_include_dir)
    
    if !File.exist?(File.join(config[:target_dir], "include"))
      
      begin
        uri = URI(include_url)
        Net::HTTP.start(uri.host, uri.port, use_ssl: uri.scheme == 'https') do |http|
          request = Net::HTTP::Get.new(uri)
          response = http.request(request)
          
          if response.code == '200'
            temp_file = File.join(target_include_dir, 'temp_include.zip')
            File.open(temp_file, 'wb') do |file|
              file.write(response.body)
            end
            
            puts "Downloaded include folder to: #{target_include_dir}"
            system("unzip -o #{temp_file} -d #{target_include_dir}")
            macosx = File.join(target_include_dir, "__MACOSX")
            FileUtils.rm_rf(macosx) if File.exist?(macosx)  
            File.delete(temp_file) if File.exist?(temp_file)
          end
        end
      end
    end
  end
    
  static_lib_config = {
    base_url: 'https://raw.githubusercontent.com/mdydek/react-native-audio-api-prebuilt-libs/main',
    lib_names: ['avcodec', 'avformat', 'avutil', 'swresample', 'ogg', 'opus', 'opusfile', 'vorbis', 'vorbisenc', 'vorbisfile'],
    target_dir: File.join(File.expand_path("..", __dir__), "common/cpp/audioapi/external")
  }

  download_static_libraries(static_lib_config)
  download_include_folder(static_lib_config)