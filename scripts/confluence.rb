#!/usr/bin/ruby

require 'json'
require 'optparse'
require 'parseconfig'
require 'rest-client'

require 'net/http'
require 'uri'


config = ParseConfig.new("#{ENV['HOME']}/.mylogin")

$username = config['default']['username']
$password = config['default']['password']
$confluence_base = config['default']['confluence_base']
$confluence_api  = $confluence_base + "/rest/api"

$options = OpenStruct.new
$options.content_file = ""
$options.details_url  = ""
$options.page_count   = ""
$options.page_title   = ""
$options.parent_url   = ""
$options.search_user  = ""
$options.update_url   = ""

class PageInfo
  attr_accessor :url
  attr_accessor :id
  attr_accessor :labels
  attr_accessor :title
  attr_accessor :space_key
  attr_accessor :version

  def initialize (url = "", id = "", title = "", space_key = "", version = 0)
    @url       = url
    @id        = id
    @labels    = []
    @title     = title
    @space_key = space_key
    @version   = version
  end

  def to_s
    puts "    URL: #{url}"
    puts "page_ID: #{id}"
    puts "  title: #{title}"
    puts "  space: #{space_key}"
    puts "version: #{version}"
    puts " labels: #{labels.join(", ")}"
  end
end

def get_details (url)
  puts "  * Looking for details of page: #{url}" if $options[:verbose]

  details = PageInfo.new(url)

  uri = URI.parse(url)
  request = Net::HTTP::Get.new(uri)
  request.basic_auth($username, $password)

  req_options = {
    use_ssl: uri.scheme == "https",
  }

  # Get whole page for header information: ID, space_key
  response = Net::HTTP.start(uri.hostname, uri.port, req_options) do |http|
    http.request(request)
  end

  if m = response.body.match( /meta\sname="ajs-page-id"\scontent="(\d*)/)
    details.id = m[1]
  else
    puts response.body if $options[:verbose]
    fail "Error: Page not found  - "#{url}"
  end

  if m = response.body.match( /meta\s*id="confluence-space-key"\s* name="confluence-space-key"\s*content="(.*)"/)
    details.space_key = m[1]
  else
    puts response.body
    fail "Error: Space key not found - #{url}"
  end

  # Get all title, version, labels via REST call
  request = "#{$confluence_api}/content/#{details.id}?expand=metadata.labels,version"
  request_full = "https://#{$username}:#{$password}@#{request}"
  begin
    json = RestClient.get(request_full) 
  rescue RestClient::ExceptionWithResponse => e
    puts e.response
    fail "Request failed: #{request}"
  end

  r = JSON.parse(json)

  #pp details
  details.title   = r['title']
  details.version = r['version']['number']
  details.labels = r['metadata']['labels']['results']
  return details
end

def get_page_count (spacename)
  puts "  * Looking for numbers of pages in space #{spacename} " if $options[:verbose]

    # Unofficial REST interface?
    url = "https://#{$username}:#{$password}@#{$confluence_base}/rest/api/search?cql=space=#{spacename} and type IN (blogpost, page)"

    begin
      json = RestClient.get(url)
    rescue RestClient::ExceptionWithResponse => e
      pp JSON.parse(e.response)
      fail "Error: The request  failed with the response seen above:"
    end
    r = JSON.parse(json)

    puts "#{r["totalSize"]} pages and blogposts in space '#{spacename}'"
end

begin
  OptionParser.new do |opts|
    opts.banner = "Usage: confluence.rb [options]"

    opts.on("-c", "--create_page PARENT_URL", "Create page. ") do |o|
      $options.parent_url << o
    end

    opts.on("-d", "--get_details PAGE_URL", "Get page details (ID and space)") do |o|
      $options.details_url << o
    end

    opts.on("-f", "--content_file FILENAME", "Content to upload") do |o|
      $options.content_file << o
    end

    opts.on("-p", "--page_count SPACENAME", "Count pages in space") do |o|
      $options.page_count << o
    end

    opts.on("-s", "--search_user USERNAME", "Search user") do |o|
      $options.search_user << o
    end

    opts.on("-t", "--page_title TITLE", "Page title for new or updated pages") do |o|
      $options.page_title << o
    end

    opts.on("-u", "--update_page PAGE_URL", "Update page, set also title (-t) and content (-f)") do |o|
      $options.update_url << o
    end

    opts.on("-v", "--verbose", "Run verbosely") do |o|
      $options[:verbose] = o
    end

    opts.on("-h", "--help", "Prints this help") do
      puts opts
      exit
    end

  end.parse!


  page_ID   = ""
  space_key = ""
 
  ### Create new page below parent_url:

  if !$options.page_count.empty?
    get_page_count ($options.page_count)
  end


  if !$options.parent_url.empty?
    puts "*** Creating new page..." if $options['verbose']

    if $options.parent_url.include?("http")
      # Create page with given parent URL
      details = get_details( $options.parent_url)
      puts details.to_s if $options['verbose']
    else
      fail "Error: Parent page does not look like an URL: #{$options.parent_url}"
    end

    if $options.page_title.empty?
      fail "Error: No page title given"
    end

    if $options.content_file.empty?
      fail "Error: No content file provided for upload"
    end

    f = File.open($options.content_file)
    page_content = f.read

    url = "https://#{$username}:#{$password}@#{$confluence_api}/content"

    payload =
      {
          "type":"page",
          "title":$options.page_title,
          "ancestors":[{"id":details.id}],
          "space":{"key":details.space_key},
          "body":{"storage": {"value":page_content, "representation":"storage"}}
      }

    #puts "payload: #{payload}.to_json" if $options['verbose']

    RestClient.log = STDOUT if $options['verbose']

    begin
      r = RestClient.post( url, payload.to_json, {content_type: :json, accept: :json} ) 
    rescue RestClient::ExceptionWithResponse => e
      pp JSON.parse(e.response)
      fail "Error: The request  failed with the response seen above:"
    end

  end

  ### Update page with update_url:

  if !$options.update_url.empty?
    puts "*** Updating #{$options.update_url}"  if $options['verbose']

    if !$options.update_url.include?("http")
      fail "Error: Page does not look like an URL: #{$options.update_url}"
    end

    if $options.content_file.empty?
      fail "Error: No content file for upload provided"
    end

    # Find ID and space key and title, which is mandatory also for page updates
    details = get_details( $options.update_url)
    details.title = $options.page_title if !$options.page_title.empty?
    puts details.to_s if $options['verbose']

    f = File.open($options.content_file)
    page_content = f.read

    url = "https://#{$username}:#{$password}@#{$confluence_api}/content/#{details.id}"

    payload =
      {
          "id":details.id,
          "type":"page",
          "title":details.title,
          "space":{"key":details.space_key},
          "version":{"number":details.version + 1},
          "body":{"storage": {"value":page_content, "representation":"storage"}}
      }

    ###########
    puts "Payload:"
    pp payload


    puts "url:"
    pp url

    puts "More, eg. verbose log:"
    ###########

    RestClient.log = STDOUT if $options['verbose']

    begin
      r = RestClient.put( url, payload.to_json, {content_type: :json} ) 
    rescue RestClient::ExceptionWithResponse => e
      pp JSON.parse(e.response)
      fail "Error: The request  failed with the response seen above:"
    end
  end

  if !$options.details_url.empty?
    details = get_details($options.details_url)
    puts details.to_s
    exit  
  end

  if !$options.search_user.empty?
    # Find users 
    puts "*** Looking for users matching: #{$options.search_user}" if $options[:verbose]
      
    # Unofficial REST interface?
    url = "https://#{$username}:#{$password}@#{$confluence_base}/rest/prototype/1/search.json?max-results=999&query=#{$options.search_user}&search=user"

    begin
      json = RestClient.get(url)
    rescue RestClient::ExceptionWithResponse => e
      pp JSON.parse(e.response)
      fail "Error: The request  failed with the response seen above:"
    end
    r = JSON.parse(json)
    pp r

    puts "#{r['result'].size} users found:" if $options[:verbose]

    r['result'].each do |u|
      puts "name: \"#{u['title']}\", login: \"#{u['username']}\", key: \"#{u['userKey']}\""
    end
    exit
  end

end
